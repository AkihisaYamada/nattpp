#ifndef _POLY_HPP
#define _POLY_HPP
#include"util.hpp"
#include"smt.hpp"
#include"map.hpp"

class Poly {
public:
	struct Error : ::Error {
		using ::Error::Error;
	};
	enum Range { NONE, POS, NEG, FULL };
	struct Var : std::string {
		Range range;
		Var( std::string_view const& str, Range range ) : std::string(str), range(range) {}
	};
	static Range range_mult( Range r1, Range r2 ) {
		switch(r1) {
		case NONE:
		case POS:
			switch(r2) {
			case NONE: return NONE;
			case POS: return POS;
			case NEG: return NEG;
			default: return FULL;
			}
		case NEG:
			switch(r2) {
			case NONE: case POS: return NEG;
			case NEG: return POS;
			default: return FULL;
			}
		default:
			return FULL;
		}
	}
	class Vars {
		std::multiset<Var> _vars;
		Range _range;
		Vars( std::multiset<Var>&& org, Range range ) : _vars(std::move(org)), _range(range) {}
	public:
		Vars() : _range(NONE) {}
		Vars( Var const& v ) : _vars{v}, _range(v.range) {}
		Range range() const {
			return _range;
		}
		std::multiset<Var> const& vars() const {
			return _vars;
		}
		Vars operator*( Vars const& vs2 ) const {
			std::multiset<Var> vs = _vars;
			vs.insert(vs2.vars().begin(),vs2.vars().end());
			return Vars( std::move(vs), range_mult(_range,vs2._range) );
		}
		auto operator==( Vars const& vs2 ) const {
			return _vars == vs2._vars;
		}
		auto operator<=>( Vars const& vs2 ) const {
			return _vars <=> vs2._vars;
		}
		friend Poly;
	};
private:
	Map<Vars,Smt::PostExp> _map;
public:
	Poly() {}
	Poly( Smt::PostExp const& c ) : _map{{{},c}} {}
	Poly( int i ) : _map{{{},i}} {}
	Poly( Var const& v ) : _map{{v,1}} {}
	Map<Vars,Smt::PostExp> const& map() const & {
		return _map;
	}
	Smt::PostExp operator[]( Vars const& vars ) const& {
		if( auto const& c = _map.find(vars) ) {
			return *c;
		}
		return 0;
	}
	/**
	 * @brief Turn coefficients into temporary variables
	 * 
	 * @param solver 
	 * @return Poly& 
	 */
	Poly& memoize( Smt::Solver& solver, Smt::BaseSort const& sort );
	Poly operator+( Poly const& p2 ) const &;
	Poly& operator+=( Poly const& p2 ) &;
	Poly monom_mult( Smt::PostExp const& c, Vars const& vs ) const;
	Poly operator*( Poly const& p2 ) const;
	Poly operator*=( Poly const& p2 ) & {
		return *this = *this * p2;
	}
	Smt::PostExp ge( Poly const& p2 ) const;
	static Smt::PostExp order( Poly const& p1, Poly const& p2, Smt::Solver& solver );
	static Algebra::Intp<Poly> algebra( Smt::Solver& solver, Smt::BaseSort const& sort );
	static Algebra::Intp<Poly> const VAR_INTP;
	static Poly sum( std::vector<Poly> const& args ) {
		Poly ret;
		for( auto const& arg : args ) {
			ret += arg;
		}
		return std::move(ret);
	}
	static Poly prod( std::vector<Poly> const& args ) {
		Poly ret = 1;
		for( auto const& arg : args ) {
			ret *= arg;
		}
		return std::move(ret);
	}
	static int test();
};

std::ostream& operator<<( std::ostream& os, Poly::Vars const& vs );

std::ostream& operator<<( std::ostream& os, Poly const& p );


#endif