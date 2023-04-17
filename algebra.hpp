#ifndef _ALGEBRA_HPP
#define _ALGEBRA_HPP

#include<cstdint>
#include"smt.hpp"
#include"trs.hpp"

class Algebra {
public:
	struct Error : Exp::Error {
		using Exp::Error::Error;
	};
	template<class T>
	class Intp : public std::function<T(std::string_view const&,std::vector<T>&&)> {
	public:
		Intp( auto const& fun ) : std::function<T(std::string_view const&,std::vector<T>&&)>(fun) {}
		T eval( Exp const& e ) const {
			if( auto const& sym = e.sym() ) {
				return (*this)(*sym,{});
			}
			if( auto const& app = e.app() ) {
				auto const& [fun,args] = *app;
				auto fsym = fun.sym();
				if( !fsym ) throw Error("#higher-order");
				std::vector<T> eargs;
				for( auto const& arg : args ) {
					eargs.push_back(eval(arg));
				}
				return (*this)(*fsym,std::move(eargs));
			}
			assert(false);
		}
	};
	static const Intp<Exp> TERM;
};

class Subst : public std::map<std::string,Exp,std::less<>> {
	template<typename T>
	T _intp( Algebra::Intp<T> const& alg, std::string_view const& f, std::vector<T>&& args ) const;
	Algebra::Intp<Exp> const _SUBST;
public:
	Subst( std::initializer_list<value_type> init ) : std::map<std::string,Exp,std::less<>>(init), _SUBST(derive(Algebra::TERM)) {}
	template<typename T>
	Algebra::Intp<T> derive( Algebra::Intp<T> const& intp ) {
		return Algebra::Intp<T>([&]( std::string_view const& f, std::vector<T>&& args ){
			return _intp(intp,f,std::move(args));
		});
	}
	Exp apply( Exp const& exp ) const {
		return _SUBST.eval(exp);
	}
};

std::ostream& operator<<( std::ostream& os, Subst const& subst );

template<typename T>
static T _intp_inner( Algebra::Intp<T> const& intp, Exp const& e, std::vector<T> const& vs ) {
	if( auto sym = e.sym() ) {
		return intp(*sym,{});
	}
	if( auto app = e.app() ) {
		auto const& [fun,args] = *app;
		if( fun == ":in" ) {// placeholder for applied variable arguments
			assert( args.size() == 1 );
			auto s = args[0].sym();
			assert(s);
			int i = stoi(*s);
			assert( i < vs.size() );
			return vs[i];
		}
		auto const& sym = fun.sym();
		assert(sym);
		std::vector<T> rargs;
		for( auto const& arg : args ) {
			rargs.push_back(_intp_inner(intp,arg,vs));
		}
		return intp(*sym,std::move(rargs));
	}
	assert(false);
}

template<typename T>
T Subst::_intp( Algebra::Intp<T> const& intp, std::string_view const& f, std::vector<T>&& vs ) const {
	if( auto it = find(f); it != end() ) {
		return _intp_inner(intp,it->second,vs);
	}
	return intp(f,std::move(vs));
}

#endif