#ifndef _ALGEBRA_HPP
#define _ALGEBRA_HPP

#include<functional>
#include"util.hpp"
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
 * @todo do not maintain function
 */
template<typename F, typename T>
struct Algebra {
	using Intp = std::move_only_function<T(F const&,std::vector<T>&&) const>;
private:
	Intp _intp;
public:
	template<typename Arg>
		requires (!std::same_as<std::remove_cvref_t<Arg>,Algebra>) && std::is_constructible_v<Intp,Arg&&>
	Algebra( Arg&& arg ) : _intp(std::forward<Arg>(arg)) {}
	Algebra(Algebra&&) = default;
	Algebra(Algebra const&) = delete;
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
	Algebra extend( Map<F,std::function<T(std::vector<T>&&)>>&& extra ) && {
		return [_intp=std::move(_intp),extra=std::move(extra)]( F const& f, std::vector<T>&& args ){
			if( auto const& fun = extra.find(f) ) {
				return fun(std::move(args));
			}
			return _intp(f,std::move(args));
		};
	}
	Algebra extend( Map<F,std::function<T(std::vector<T>&&)>>&& extra ) const& {
		return [&,extra=std::move(extra)]( F const& f, std::vector<T>&& args ){
			if( auto const& fun = extra.find(f) ) {
				return (*fun)(std::move(args));
			}
			return _intp(f,std::move(args));
		};
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
		if( auto i = ifun.template ref<Arg>() ) {// placeholder for applied variable arguments
			assert( args.empty() );
			if( i->pos() >= vs.size() ) throw Error("#deriver:too-few-arguments");
			return vs[i->pos()];
		}
		if( auto fun = ifun.template ref<G>() ) {
			std::vector<T> vargs;
			for( auto const& arg : args ) {
				vargs.push_back(_intp_inner(org,arg,vs));
			}
			return org(*fun,std::move(vargs));
		}
		assert(false);
	};
public:
	template<typename... Args>
		requires std::is_constructible_v<_Map,Args&&...>
	Deriver( Args&&... args ) : _map(std::forward<Args>(args)...) {}
	Deriver( std::initializer_list<typename _Map::value_type> list ) : _map(list) {}
	Opt<ArgTerm<G> const&> find( F const& f ) const& {
		return _map.find(f);
	}
	_Map const& map() const& { return _map; }
	Deriver& assign( F const& f, ArgTerm<G> const& t ) {
		_map.emplace(f,t);
		return *this;
	}
	/** general substitution */
	Term<G> subst( Term<F> const& t ) const& { return derive(TERM<G>)(t); }
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
		return os << "(arg " << a->pos() << ')';
	}
	assert(false);
}

template<typename F, typename G>
std::ostream& operator<<( std::ostream& os, Deriver<F,G> const& subst ) {
	os << "(deriver" << std::endl;
	for( auto const& [key,val] : subst.map() ) {
		os << "  (" << key << ' ' << val << ')' << std::endl;
	}
	return os << ')';
}

template<typename F>
std::ostream& operator<<( std::ostream& os, Subst<F> const& subst ) {
	os << "(subst" << std::endl;
	for( auto const& [key,val] : subst ) {
		os << "  (" << key << ' ' << val << ')' << std::endl;
	}
	return os << ')';
}

#endif