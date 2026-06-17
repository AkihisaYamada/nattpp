#ifndef _POLY_HPP
#define _POLY_HPP
#include"util.hpp"
#include"smt.hpp"
#include"template.hpp"

struct Poly {
	struct Error : ::Error {
		using ::Error::Error;
	};
	enum Range { NONE, POS, NEG, FULL };
	struct Add {};
	static Add constexpr ADD = {};
	struct Mul {};
	static Mul constexpr MUL = {};
	struct Cond { Smt::PreExp exp; };
	struct Var : std::string {
		Range range;
		Var( std::string_view const& str, Range range ) : std::string(str), range(range) {}
	};
	using Sig = Sum<Add,Mul,Cond,Smt::PreExp,Var>;
	static Range range_mult( Range r1, Range r2 ) {
		switch(r1) {
		case NONE:
			return r2;
		case POS:
			switch(r2) {
			case NONE: case POS: return POS;
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
	struct Vars {
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
	Map<Vars,Smt::PreExp> _map;
public:
	Poly() {}
	template<typename T> requires std::is_constructible_v<Smt::PreExp,T>
	Poly( T&& c ) : _map{{{},Smt::PreExp(std::forward<T>(c))}} {}
	Poly( Var const& v ) : _map{{v,Smt::PreExp(1)}} {}
	Map<Vars,Smt::PreExp> const& map() const & {
		return _map;
	}
	Smt::PreExp operator[]( Vars const& vars ) const& {
		if( auto const& c = _map.find(vars) ) {
			return *c;
		}
		return Smt::PreExp(0);
	}
	friend Poly operator+( Poly const& p1, Poly const& p2 );
	friend Poly& operator+=( Poly& p1, Poly const& p2 );
	Poly monom_mult( Smt::PreExp const& c, Vars const& vs ) const;
	friend Poly operator*( Poly const& p1, Poly const& p2 );
	friend Poly& operator*=( Poly& p1, Poly const& p2 ) {
		return p1 = p1 * p2;
	}
	Smt::PreExp ge( Poly const& p2 ) const;
	friend Poly ite( Poly const& c, Poly const& p1, Poly const& p2 );
	friend Smt::Compare order( Poly const& p1, Poly const& p2, Smt::Solver& solver );
	static int test();
};

struct MPoly {
private:
	std::vector<Poly> _set;
public:
	/** -∞ */
	MPoly() {}
	template<typename T> requires std::is_constructible_v<Poly,T>
	MPoly( T&& arg ) { _set.emplace_back( std::forward<T>(arg) ); }
	MPoly& join( MPoly const& y ) & {
		for( auto const& yc : y._set ) {
			_set.emplace_back(yc);
		}
		return *this;
	}
	std::vector<Poly> const& set() const& {
		return _set;
	}
	static Algebra<Template::Fun,MPoly> const ALGEBRA;
	friend MPoly ite( MPoly const& c, MPoly const& p1, MPoly const& p2 );
	friend MPoly operator+( MPoly const& x, MPoly const& y );
	friend MPoly& operator+=( MPoly& x, MPoly const& y ) {
		return x = x + y;
	}
	friend MPoly operator*( MPoly const& x, MPoly const& y );
	friend MPoly& operator*=( MPoly& x, MPoly const& y ) {
		return x = x * y;
	}
	Smt::PreExp ge( MPoly const& p2 ) const;
	friend Smt::Compare order( MPoly const& x, MPoly const& y, Smt::Solver& solver );
};

std::ostream& operator<<( std::ostream& os, Poly::Sig const& f );

std::ostream& operator<<( std::ostream& os, Poly::Range const& r );

std::ostream& operator<<( std::ostream& os, Poly::Var const& v );

std::ostream& operator<<( std::ostream& os, Poly::Vars const& vs );

std::ostream& operator<<( std::ostream& os, Poly const& p );

std::ostream& operator<<( std::ostream& os, MPoly const& p );

#endif