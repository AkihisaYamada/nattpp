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
	class Add {};
	static Add constexpr ADD = {};
	class Mul {};
	static Mul constexpr MUL = {};
	struct Cond { Smt::PreExp exp; };
	struct Var : std::string {
		Range range;
		Var( std::string_view const& str, Range range ) : std::string(str), range(range) {}
	};
	using Sig = Sum<Add,Mul,Cond,Smt::PreExp,Var>;
	static Algebra::Intp<Sig,Smt::PreExp> const ALGEBRA;
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
	Map<Vars,Smt::PreExp> _map;
public:
	Poly() {}
	template<typename T> requires std::is_constructible_v<Smt::PreExp,T>
	Poly( T const& c ) : _map{{{},c}} {}
	Poly( int i ) : _map{{{},Smt::PreExp(i)}} {}
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
	/**
	 * @brief Turn coefficients into temporary variables
	 * 
	 * @param solver 
	 * @return Poly& 
	 */
	Poly& expand( Smt::Solver& solver, Smt::BaseSort const& sort );
	/** evaluate SMT coefficients */
	Poly eval_coeffs( Smt::Solver& solver ) const;
	Poly operator+( Poly const& p2 ) const &;
	Poly& operator+=( Poly const& p2 ) &;
	Poly monom_mult( Smt::PreExp const& c, Vars const& vs ) const;
	Poly operator*( Poly const& p2 ) const;
	Poly operator*=( Poly const& p2 ) & {
		return *this = *this * p2;
	}
	Smt::PreExp ge( Poly const& p2 ) const;
	static Poly ite( Smt::PreExp const& c, Poly const& p1, Poly const& p2 );
	static Smt::PreExp order( Poly const& p1, Poly const& p2, Smt::Solver& solver );
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

	template<typename F>
	static Algebra::Intp<F,Poly> expand( Algebra::Intp<F,Poly> && intp, Smt::Solver& solver, Smt::BaseSort const& sort ) {
		return [intp=std::move(intp),&solver,sort]( F const& f, std::vector<Poly> && args ) {
			return intp(f,std::move(args)).expand(solver,sort);
		};
	}

	template<typename F>
	static Algebra::Intp<F,Poly> expand( Algebra::Intp<F,Poly> const& intp, Smt::Solver& solver, Smt::BaseSort const& sort ) {
		return [&intp,&solver,sort]( F const& f, std::vector<Poly> && args ) {
			return intp(f,std::move(args)).expand(solver,sort);
		};
	}

	static Algebra::Intp<Sig,Poly> algebra( Smt::Solver& solver );

	class Template : public Exp {
	public:
		Template( Exp const& exp ) : Exp(exp) {}
		Algebra::Deriver<std::string,Sig> deriver(
			Trs::Sig const& sig,
			Smt::Solver& solver
		) const;
		static Template const SUM;
		static Template const MONO_SUM;
	private:
		static Term<Sum<Sig,Algebra::Arg>> _deriver_inner(
			std::string const& f,
			Trs::Rank const& rank,
			Smt::Solver& solver,
			Exp const& exp,
			int pos
		);
	};

	static Algebra::Template<Sig> eval_coeff(
		Smt::Solver& solver,
		Algebra::Template<Sig> const& org
	);
};

std::ostream& operator<<( std::ostream& os, Poly::Sig const& f );

std::ostream& operator<<( std::ostream& os, Poly::Range const& r );

std::ostream& operator<<( std::ostream& os, Poly::Var const& v );

std::ostream& operator<<( std::ostream& os, Poly::Vars const& vs );

std::ostream& operator<<( std::ostream& os, Poly const& p );

#endif