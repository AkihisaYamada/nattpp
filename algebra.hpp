#ifndef _ALGEBRA_HPP
#define _ALGEBRA_HPP

#include<cstdint>
#include"map.hpp"
#include"trs.hpp"

class Algebra {
public:
	struct Error : Exp::Error {
		using Exp::Error::Error;
	};
	template<class T>
	struct Intp : std::function<T(std::string_view const&,std::vector<T>&&)> {
		Intp( auto const& fun ) : std::function<T(std::string_view const&,std::vector<T>&&)>(fun) {}
		T eval( Exp const& e ) const {
			std::vector<T> vargs;
			for( auto const& arg : e.args() ) {
				vargs.push_back(eval(arg));
			}
			return (*this)(e.fun(),std::move(vargs));
		}
	};
	/** @brief The term algebra */
	static Intp<Exp> const TERM;
};

template<typename T>
static T _intp_inner( Algebra::Intp<T> const& intp, Exp const& e, std::vector<T> const& vs ) {
	auto const& fun = e.fun();
	auto const& args = e.args();
	if( fun == ":in" ) {// placeholder for applied variable arguments
		assert( args.size() == 1 );
		assert( args[0].args().size() == 0 );
		int i = stoi(args[0].fun());
		assert( i < vs.size() );
		return vs[i];
	}
	std::vector<T> vargs;
	for( auto const& arg : args ) {
		vargs.push_back(_intp_inner(intp,arg,vs));
	}
	return intp(fun,std::move(vargs));
}

class Deriver {
public:
	class Map;
private:
	template<typename T>
	T _intp(
		Algebra::Intp<T> const& intp,
		Algebra::Intp<T> const& default_intp,
		std::string_view const& f,
		std::vector<T>&& args
	) const;
	Algebra::Intp<Exp> const _SUBST;
public:
	Deriver() : _SUBST(derive(Algebra::TERM,Algebra::TERM)) {}
	virtual Opt<Exp const&> find( std::string_view const& ) const = 0;
	template<typename T>
	Algebra::Intp<T> derive( Algebra::Intp<T> const& intp, Algebra::Intp<T> const& default_intp ) & {
		return [&]( std::string_view const& f, std::vector<T>&& args ){
			return _intp(intp,default_intp,f,std::move(args));
		};
	}
	Exp subst( Exp const& exp ) const {
		return _SUBST.eval(exp);
	}
};

template<typename T>
T Deriver::_intp(
	Algebra::Intp<T> const& intp,
	Algebra::Intp<T> const& default_intp,
	std::string_view const& f,
	std::vector<T>&& vs
) const {
	if( auto e = find(f) ) {
		return _intp_inner(intp,*e,vs);
	}
	return default_intp(f,std::move(vs));
}

class Deriver::Map : public ::Map<std::string,Exp>, public Deriver {
public:
	Map() {}
	Map( std::initializer_list<value_type> list ) : ::Map<std::string,Exp>(list) {}
	Opt<Exp const&> find( std::string_view const& f ) const {
		return ::Map<std::string,Exp>::find(f);
	}
};

std::ostream& operator<<( std::ostream& os, Deriver::Map const& subst );

#endif