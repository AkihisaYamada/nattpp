#ifndef _POLY_HPP
#define _POLY_HPP
#include"util.hpp"
#include"smt.hpp"
#include"template.hpp"

struct Poly;
struct MPoly;

std::ostream& operator<<( std::ostream& os, Poly const& p );
std::ostream& operator<<( std::ostream& os, MPoly const& p );

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
		Var( Var const& other ) = default;
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
	struct VarSet : Map<std::string,Range> {
		using Map::Map;
		Smt::PostExp quantify( Smt::BaseSort sort, Smt::PostExp const& body ) const {
			auto ret = body;
			for( auto const& [var,range] : *this ) {
				ret = Smt::Forall(var,sort) >>= [&]{
					switch(range){
						case POS: return Smt::PostExp(Smt::Local(var)).ge(0);
						case NEG: return Smt::PostExp(Smt::Local(var)).le(0);
						default: return Smt::PostExp(true);
					}
				}().imp(ret);
			}
			return ret;
		}
	};
	struct VarProd {
		std::multiset<Var> _vars;
		Range _range;
		VarProd( std::multiset<Var>&& org, Range range ) : _vars(std::move(org)), _range(range) {}
	public:
		VarProd() : _range(NONE) {}
		VarProd( Var const& v ) : _vars{v}, _range(v.range) {}
		Range range() const {
			return _range;
		}
		std::multiset<Var> const& vars() const {
			return _vars;
		}
		VarProd operator*( VarProd const& vs2 ) const {
			std::multiset<Var> vs = _vars;
			vs.insert(vs2.vars().begin(),vs2.vars().end());
			return VarProd( std::move(vs), range_mult(_range,vs2._range) );
		}
		auto operator==( VarProd const& vs2 ) const {
			return _vars == vs2._vars;
		}
		auto operator<=>( VarProd const& vs2 ) const {
			return _vars <=> vs2._vars;
		}
		Smt::PreExp as_smt() const;
		friend Poly;
	};
private:
	OrdMap<VarProd,Smt::PreExp> _map;
	VarSet _vars;
public:
	Poly() {}
	template<typename T> requires std::is_constructible_v<Smt::PreExp,T>
	Poly( T&& c ) : _map{{{},Smt::PreExp(std::forward<T>(c))}} {}
	Poly( Var const& v ) : _map{{v,1}}, _vars{{v,v.range}} {}
	OrdMap<VarProd,Smt::PreExp> const& map() const & {
		return _map;
	}
	Smt::PreExp operator[]( VarProd const& vars ) const& {
		if( auto const& c = _map.find(vars) ) {
			return *c;
		}
		return Smt::PreExp(0);
	}
	void set_coeff( VarProd const& vars, Smt::PreExp const& coeff ) {
		if( _map.emplace(vars,coeff).second ) {
			for( auto const& var : vars._vars ) {
				_vars.emplace(var,var.range);
			}
		}
	}
	void memoize( Smt::Solver& solver )& {
		for( auto& [vs,coeff] : _map ) {
			coeff = solver.let(coeff);
		}
	}
	friend Poly operator+( Poly const& p1, Poly const& p2 );
	friend Poly& operator+=( Poly& p1, Poly const& p2 );
	Poly monom_mult( Smt::PreExp const& c, VarProd const& vs ) const;
	friend Poly operator*( Poly const& p1, Poly const& p2 );
	friend Poly& operator*=( Poly& p1, Poly const& p2 ) {
		return p1 = p1 * p2;
	}
	Smt::PostExp ge( Poly const& p2 ) const;
	Smt::PostExp gt( Poly const& p2 ) const;
	Smt::Compare compare( Poly const& p2, Smt::Solver& solver ) const;
	Smt::PostExp separate( Poly const& p2 ) const;
	Smt::PreExp as_smt() const;
	VarSet vars() && { return std::move(_vars); }
	VarSet const& vars() const& { return _vars; }
	friend Poly ite( Poly const& c, Poly const& p1, Poly const& p2 );
	static Algebra<Template::Sym,Poly> algebra_of_range( Range ran );
	static Algebra<Template::Sym,Poly> const ALGEBRA;
	static Algebra<Template::Sym,Poly> const POS_ALGEBRA;
	static int test();
};

struct MPoly {
private:
	std::vector<Poly> _set;
public:
	using Var = Poly::Var;
	/** -∞ */
	MPoly() {}
	template<typename T> requires std::is_constructible_v<Poly,T>
	MPoly( T&& arg ) { _set.emplace_back( std::forward<T>(arg) ); }
	std::vector<Poly> const& set() const& {
		return _set;
	}
	static Algebra<Template::Sym,MPoly> algebra_of_range( Poly::Range ran );
	static Algebra<Template::Sym,MPoly> const ALGEBRA, POS_ALGEBRA;
	friend MPoly ite( MPoly const& c, MPoly const& p1, MPoly const& p2 );
	friend MPoly operator+( MPoly const& x, MPoly const& y );
	friend MPoly operator*( MPoly const& x, MPoly const& y );
	friend MPoly& max_eq( MPoly& x, MPoly const& y );
	Smt::PostExp ge( MPoly const& p2 ) const;
	Smt::PostExp gt( MPoly const& p2 ) const;
	Smt::Compare compare( MPoly const& y, Smt::Solver& solver ) const;
	Smt::PostExp separate( MPoly const& y ) const;
	void memoize( Smt::Solver& solver )& {
		for( auto& p : _set ) {
			p.memoize(solver);
		}
	}
};
MPoly operator+( MPoly const& x, MPoly const& y );
inline MPoly& operator+=( MPoly& x, MPoly const& y ) {
	return x = x + y;
}
MPoly operator*( MPoly const& x, MPoly const& y );
inline MPoly& operator*=( MPoly& x, MPoly const& y ) {
	return x = x * y;
}
inline MPoly& max_eq( MPoly& x, MPoly const& y ) {
	for( auto const& yc : y._set ) {
		x._set.emplace_back(yc);
	}
	return x;
}

std::ostream& operator<<( std::ostream& os, Poly::Sig const& f );

std::ostream& operator<<( std::ostream& os, Poly::Range const& r );

std::ostream& operator<<( std::ostream& os, Poly::Var const& v );

std::ostream& operator<<( std::ostream& os, Poly::VarProd const& vs );

#endif