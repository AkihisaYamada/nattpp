#ifndef _ALGEBRA_HPP
#define _ALGEBRA_HPP

#include"map.hpp"
#include"exp.hpp"

/** @brief For Deriver: Placeholder for argument position. */
class Arg {
	int _pos;
public:
	Arg( int pos ) : _pos(pos) {}
	int pos() const { return _pos; }
};

/**
 * @brief Algebra
 * 
 * @tparam F signature
 * @tparam T carrier
 */
template<typename F, typename T>
struct Algebra {
	using Intp = std::function<T(F const&,std::vector<T>&&)>;
private:
	Intp _intp;
public:
	template<typename... Args>
		requires std::is_constructible_v<Intp,Args&&...>
	Algebra( Args&&... args ) : _intp(std::forward<Args>(args)...) {}
	T operator()( F const& f, std::vector<T>&& args ) const {
		return _intp(f,std::move(args));
	}
	T operator()( Term<F> const& e ) const {
		std::vector<T> vargs;
		for( auto const& arg : e.args() ) {
			vargs.push_back(operator()(arg));
		}
		return _intp(e.fun(),std::move(vargs));
	}
	using ASig = std::pair<F,T>;
	using ATerm = Term<ASig>;
	/**
	 * @brief Annotates a term with its evaluation
	 */
	ATerm annotate( Term<F> const& e ) const& {
		std::vector<ATerm> aargs;
		std::vector<T> vargs;
		for( auto& arg : e.args() ) {
			ATerm aarg = annotate(arg);
			vargs.push_back(aarg.fun().second);
			aargs.push_back(std::move(aarg));
		}
		T v = _intp( e.fun(), std::move(vargs) );
		return ATerm(std::in_place,ASig(e.fun(),std::move(v)),std::move(aargs));
	}
};

/** @brief The term algebra */
template<typename F>
Algebra<F,Term<F>> const TERM = []( F const& f, std::vector<Term<F>>&& args ){
	return app(f,std::move(args));
};

template<typename G>
using ArgTerm = Term<Sum<G,Arg>>;

template<typename F, typename G>
struct Deriver {
private:
	using _Map = Map<F,ArgTerm<G>>;
	_Map _map;
	template<typename T>
	static T _intp_inner( Algebra<G,T> const& org, ArgTerm<G> const& e, std::vector<T> const& vs ) {
		auto const& [ifun,args] = *e;
		if( auto i = ifun.template ref<1>() ) {// placeholder for applied variable arguments
			assert( args.empty() );
			if( i->pos() >= vs.size() ) throw Error("#deriver:too-few-arguments");
			return vs[i->pos()];
		}
		if( auto fun = ifun.template ref<0>() ) {
			std::vector<T> vargs;
			for( auto const& arg : args ) {
				vargs.push_back(_intp_inner(org,arg,vs));
			}
			return org(*fun,std::move(vargs));
		}
		assert(false);
	};
public:
	Algebra<F,Term<G>> const algebra = [&]( F const& f, std::vector<Term<G>>&& args ){
		if( auto const& df = _map.find(f) ) {
			return _intp_inner(TERM<G>,*df,std::move(args));
		}
		return app(G(f),std::move(args));
	};
	template<typename... Args>
		requires std::is_constructible_v<_Map,Args&&...>
	Deriver( Args&&... args ) : _map(std::forward<Args>(args)...) {}
	Deriver( std::initializer_list<typename _Map::value_type> list ) : _map(list) {}
	Opt<ArgTerm<G> const&> find( F const& f ) const& {
		return _map.find(f);
	}
	/** general substitution */
	Term<G> subst( Term<F> const& t ) const& { return algebra(t); }
	auto derive( auto ) && = delete;
	template<typename T>
	Algebra<F,T> derive( Algebra<G,T>&& org ) const & {
		return [org=std::move(org),this]( F const& f, std::vector<T>&& args ){
			if( auto const& df = _map.find(f) ) {
				return _intp_inner(org,*df,std::move(args));
			}
			return org(f,std::move(args));
		};
	}
	template<typename T>
	Algebra<F,T> derive( Algebra<G,T> const& org ) const & {
		return [&]( F const& f, std::vector<T>&& args ){
			if( auto const& df = _map.find(f) ) {
				return _intp_inner(org,*df,std::move(args));
			}
			return org(f,std::move(args));
		};
	}
};


template<typename F>
struct Subst {
private:
	Map<F,Term<F>> _map;
	Algebra<F,Term<F>> _alg = [&]( F const& f, std::vector<Term<F>>&& args ){
		if( auto const& t = _map.find(f) ) {
			return *t;
		}
		return app(f,std::move(args));
	};
public:
	template<typename... Args>
	Subst( Args&&... args ) : _map(std::forward<Args>(args)...) {}
	Subst( std::initializer_list<typename Map<F,Term<F>>::value_type> list ) :
		_map(list) {}
	Term<F> operator()( Term<F> const& s ) const& {
		return _alg(s);
	}
};

template<typename F>
std::ostream& operator<<( std::ostream& os, Sum<F,Arg> const& df ) {
	if( auto const& f = df.template ref<F>() ) {
		return os << *f;
	}
	if( auto const& a = df.template ref<Arg>() ) {
		return os << "(arg " << a->pos()+1 << ')';
	}
	assert(false);
}

template<typename F>
std::ostream& operator<<( std::ostream& os, Subst<F> const& subst ) {
	os << '[' << std::endl;
	for( auto const& [key,val] : subst ) {
		os << '\t' << key << " := " << val << std::endl;
	}
	return os << ']';
}

#endif