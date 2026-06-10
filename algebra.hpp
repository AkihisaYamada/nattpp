#ifndef _ALGEBRA_HPP
#define _ALGEBRA_HPP

#include<cstdint>
#include"map.hpp"
#include"trs.hpp"

class Algebra {
	Algebra() = delete;
public:
	struct Error : ::Error {
		using ::Error::Error;
	};
	/**
	 * @brief Interpretation
	 * 
	 * @tparam F signature
	 * @tparam T carrier
	 */
	template<typename F, typename T>
	struct Intp : public std::function<T(F const&,std::vector<T>&&)> {
		using std::function<T(F const&,std::vector<T>&&)>::function;
		T eval( Term<F> const& e ) const {
			std::vector<T> vargs;
			for( auto const& arg : e.args() ) {
				vargs.push_back(eval(arg));
			}
			return (*this)(e.fun(),std::move(vargs));
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
			T v = (*this)( e.fun(), std::move(vargs) );
			return ATerm(std::in_place,ASig(e.fun(),std::move(v)),std::move(aargs));
		}
	};

	/** @brief The term algebra */
	template<typename F>
	static Intp<F,Term<F>> const TERM;

	/** @brief For Deriver: Placeholder for argument position. */
	class Arg {
		friend Algebra;
		int _pos;
	public:
		Arg( int pos ) : _pos(pos) {}
		int pos() const { return _pos; }
	};
	/** @brief For Deriver: Expression with argument placeholders */
	template<typename G>
	using Template = Term<Sum<G,Arg>>;

	template<typename F, typename G>
	struct Deriver : std::function<Term<Sum<G,Arg>>(F const&)> {
		using std::function<Term<Sum<G,Arg>>(F const&)>::function;
		template<typename T>
		Intp<F,T> derive( auto ) && = delete;
		template<typename T>
		Intp<F,T> derive( Intp<G,T> && intp ) const & {
			return [intp=std::move(intp),this]( F const& f, std::vector<T>&& args ){
				return _intp_inner(intp,(*this)(f),std::move(args));
			};
		}
		template<typename T>
		Intp<F,T> derive( Intp<G,T> const& intp ) const & {
			return [&]( F const& f, std::vector<T>&& args ){
				return _intp_inner(intp,(*this)(f),std::move(args));
			};
		}
	private:
		template<typename T>
		static T _intp_inner( Intp<G,T> const& intp, Term<Sum<G,Arg>> const& e, std::vector<T> const& vs ) {
			Sum<G,Arg> const& ifun = e.fun();
			auto const& args = e.args();
			if( auto i = ifun.template ref<1>() ) {// placeholder for applied variable arguments
				assert( args.empty() );
				assert( i->_pos < vs.size() );
				return vs[i->_pos];
			}
			if( auto fun = ifun.template ref<0>() ) {
				std::vector<T> vargs;
				for( auto const& arg : args ) {
					vargs.push_back(_intp_inner(intp,arg,vs));
				}
				return intp(*fun,std::move(vargs));
			}
			assert(false);
		};
	};

	static int test();
};

template<typename F>
Algebra::Intp<F,Term<F>> const Algebra::TERM = []( F const& f, std::vector<Term<F>>&& args ){
	return app(f,std::move(args));
};

template<typename F>
struct Subst : Map<F,Term<F>>, Algebra::Intp<F,Term<F>> {
	Subst( std::initializer_list<typename Map<F,Term<F>>::value_type> list ) :
		Map<F,Term<F>>(list),
		Algebra::Intp<F,Term<F>>([&]( F const& f, std::vector<Term<F>>&& args ){
			if( auto const& t = Map<F,Term<F>>::find(f) ) {
				return *t;
			}
			return app(f,std::move(args));
		})
	{}
};

template<typename F>
std::ostream& operator<<( std::ostream& os, Sum<F,Algebra::Arg> const& df ) {
	if( auto f = df.template ref<F>() ) {
		return os << *f;
	}
	if( auto a = df.template ref<Algebra::Arg>() ) {
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