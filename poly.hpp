#ifndef _POLY_HPP
#define _POLY_HPP
#include"util.hpp"
#include"algebra.hpp"
#include"map.hpp"

class Poly {
public:
	enum Range { POS, NEG, FULL };
	struct Var : std::string {
		Range range;
		Var( std::string_view const& str, Range range ) : std::string(str), range(range) {}
	};
	static Range range_mult( Range r1, Range r2 ) {
		switch(r1) {
		case POS:
			switch(r2) {
			case POS: return POS;
			case NEG: return NEG;
			default: return FULL;
			}
		case NEG:
			switch(r2) {
			case POS: return NEG;
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
		Vars() : _range(POS) {}
		Vars( Var const& v ) : _vars{v}, _range(v.range) {}
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
	Map<Vars,Smt::Exp> _map;
public:
	Poly() {}
	Poly( Smt::Exp const& c ) : _map{{{},c}} {}
	Poly( int i ) : _map{{{},i}} {}
	Poly( Var const& v ) : _map{{v,1}} {}
	Map<Vars,Smt::Exp> const& map() const & {
		return _map;
	}
	Poly operator+( Poly const& p2 ) const & {
		Poly ret;
		iter2(_map,p2._map,[&]( auto& it1, auto& it2 ){
			ret._map.insert( it1->first, it1->second + it2->second );
		},[&]( auto& it1 ){
			ret._map.insert(*it1);
		},[&]( auto& it2 ){
			ret._map.insert(*it2);
		});
		return std::move(ret);
	}
	Poly& operator+=( Poly const& p2 ) & {
		iter2(_map,p2._map,[&]( auto it1, auto it2 ){
			it1->second += it2->second;
		},[&]( auto it1 ){
		},[&]( auto it2 ){
			_map.insert(*it2);
		});
		return *this;
	}
	Poly monom_mult( Smt::Exp const& c, Vars const& vs ) const {
		Poly ret;
		for( auto& [vs1,c1] : _map ) {
			ret._map.insert(vs1 * vs, c * c1);
		}
		return std::move(ret);
	}
	Poly operator*( Poly const& p2 ) const {
		Poly ret;
		for( auto const& [vs2,c2] : p2._map ) {
			ret += monom_mult(c2,vs2);
		}
		return std::move(ret);
	}
	Poly operator*=( Poly const& p2 ) & {
		return *this = *this * p2;
	}
	Smt::Exp ge( Poly const& p2 ) const {
		Smt::Exp ge = Smt::TRUE;
		iter2(_map,p2._map,[&]( auto it1, auto it2 ){
			switch( it1->first._range ) {
				case POS: ge = ge && it1->second.ge(it2->second); return;
				case NEG: ge = ge && it2->second.ge(it1->second); return;
				case FULL: ge = ge && it1->second.eq(it2->second); return;
			}
		},[&]( auto it1 ){
			if( it1->first._range != POS ) {
				ge = ge && it1->second.eq(0);
			}
		},[&]( auto it2 ){
			if( it2->first._range != NEG ) {
				ge = ge && it2->second.eq(0);
			}
		});
		return ge;
	}
	static Algebra::Intp<Poly> const ALGEBRA;
	static Poly sum( std::vector<Poly> const& args ) {
		Poly ret;
		for( auto const& arg : args ) {
			ret += arg;
		}
		return std::move(ret);
	}
	static Poly prod( std::vector<Poly> const& args ) {
		Poly ret;
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