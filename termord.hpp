#ifndef TERMORD_HPP_
#define TERMORD_HPP_

#include<list>
#include"smt.hpp"

struct TermOrder {
	virtual std::ostream& print_name( std::ostream& os ) = 0;
	virtual std::ostream& print( std::ostream& os, Trs::Sig const& sig ) = 0;
	virtual std::pair<Smt::PostExp,Smt::PostExp> compare( Exp const& l, Exp const& r ) = 0;
};

struct TrivOrder : TermOrder {
	std::pair<Smt::PostExp,Smt::PostExp> compare( Exp const& l, Exp const& r ) override {
		return {Smt::TRUE,Smt::FALSE};
	};
	std::ostream& print_name( std::ostream& os ) override {
		return os << "trivial-order";
	};
	std::ostream& print( std::ostream& os, Trs::Sig const& sig ) override {
		return os;
	}
};

template<typename A>
struct DerivedTermOrder : TermOrder {
	Smt::Solver solver;
	DerivedTermOrder( DerivedTermOrder const& ) = delete;
	DerivedTermOrder( DerivedTermOrder && ) = default;
	Algebra::Intp<std::string,A> const intp;
	Algebra::Deriver<std::string, typename A::Sig> const deriver;
	DerivedTermOrder(
		Trs::Sig const& sig,
		A::Template const& temp,
		Smt::Solver&& sol,
		Smt::Sort const& sort
	) : solver(std::move(sol)),
		deriver(temp.deriver(sig,solver)),
		intp(A::expand(deriver.derive(A::algebra(solver)),solver,sort)) {
	}
	std::pair<Smt::PostExp,Smt::PostExp> compare( Exp const& l, Exp const& r ) override {
		return A::compare(intp.eval(l),intp.eval(r),solver);
	}
	std::ostream& print_name( std::ostream& os ) override {
		return os << "derived-order";
	};
	std::ostream& print( std::ostream& os, Trs::Sig const& sig ) override {
		os << '(';
		for( auto [f,arity] : sig ) {
			os << "\n    (" << f << ' ' << A::instantiate(solver,deriver(f)) << ')';
		}
		return os << ')';
	}
};

struct TrsOrder : TermOrder {
	virtual Smt::Solver& solver() = 0;
	virtual std::pair<Smt::PostExp,Smt::PostExp> order_rule( size_t i ) & = 0;
};

std::vector<size_t> order_some_rule( TrsOrder& order, Trs::Rules const& rules );

template<typename A>
struct DerivedTrsOrder : TrsOrder {
private:
	DerivedTermOrder<A> _term_order;
	Map<size_t,std::pair<Smt::PostExp,Smt::PostExp>> _ords;
public:
	DerivedTrsOrder(
		Trs::Sig const& sig,
		Trs::Rules& rules,
		A::Template const& temp,
		Smt::Solver&& solver,
		Smt::Sort const& sort
	) : _term_order(sig,temp,std::move(solver),sort) {
		for( auto const& [i,rule] : rules ) {
			auto [ge,gt] = _term_order.compare(rule.first,rule.second);
			auto gev = _term_order.solver.let(Smt::BOOL,ge);
			auto gtv = _term_order.solver.let(Smt::BOOL,gt);
			_ords.insert(i,std::pair(gev,gtv));
		}
	}
	Smt::Solver& solver() override { return _term_order.solver; }
	std::pair<Smt::PostExp,Smt::PostExp> order_rule( size_t i ) & override {
		auto o = _ords.find(i);
		assert(o);
		return *o;
	}
	std::pair<Smt::PostExp,Smt::PostExp> compare( Exp const& l, Exp const& r ) override {
		return _term_order.compare(l,r);
	}
	std::ostream& print_name( std::ostream& os ) override {
		return _term_order.print_name(os);
	}
	std::ostream& print( std::ostream& os, Trs::Sig const& sig ) override {
		return _term_order.print(os,sig);
	}
};

struct TrsPosOrder : TrsOrder {
	virtual std::pair<Smt::PostExp,Smt::PostExp> order_rule( size_t i, Pos const& l, Pos const& r ) & = 0;
	std::pair<Smt::PostExp,Smt::PostExp> order_rule( size_t i ) & override {
		return order_rule(i,{},{});
	}
};

template<typename A>
struct DerivedTrsPosOrder : TrsPosOrder {
	using ASig = std::pair<std::string,A>;
protected:
	DerivedTermOrder<A> _term_order;
	Map<size_t,std::pair<Term<ASig>,Term<ASig>>> _arules;
	DerivedTrsPosOrder( DerivedTrsPosOrder const& ) = delete;
public:
	DerivedTrsPosOrder( DerivedTrsPosOrder && ) = default;
	DerivedTrsPosOrder(
		Trs::Sig const& sig,
		Trs::Rules const& rules,
		A::Template const& temp,
		Smt::Solver&& solver,
		Smt::BaseSort const& sort
	) : _term_order(sig,temp,std::move(solver),sort) {
		for( auto [n,rule] : rules ) {
			_arules.insert(n,std::pair{_term_order.intp.annotate(rule.first),_term_order.intp.annotate(rule.second)});
		}
	}
	Smt::Solver& solver() override { return _term_order.solver; }
	std::pair<Smt::PostExp,Smt::PostExp> order_rule( size_t i, Pos const& lpos, Pos const& rpos ) & override {
		auto arule = _arules.find(i);
		assert(arule);
		auto lv = arule->first.at(lpos).fun().second;
		auto rv = arule->second.at(rpos).fun().second;
		return A::compare(lv,rv,solver());
	}
	std::pair<Smt::PostExp,Smt::PostExp> compare( Exp const& l, Exp const& r ) override {
		return _term_order.compare(l,r);
	}
	std::ostream& print_name( std::ostream& os ) override {
		return _term_order.print_name(os);
	}
	std::ostream& print( std::ostream& os, Trs::Sig const& sig ) override {
		return _term_order.print(os,sig);
	}
};

class PathOrder : public TermOrder {
	struct SigInfo {
		Smt::PostExp prec;
	};
	Map<std::string,SigInfo> _map;
public:
	TermOrder& weight;
	PathOrder( Trs::Sig const& sig, TermOrder& weight, Smt::Solver& solver ) : weight(weight) {
		for( auto it1 = sig.begin(); it1 != sig.end(); ) {
			auto const& [fun1,rank1] = *it1;
			for( auto it2 = sig.begin(); it2 != it1; it2++ ) {
				_map.insert(fun1,SigInfo{solver.declare_fresh(Smt::INT)});
				
			}
		}
	}
	std::pair<Smt::PostExp,Smt::PostExp> compare( Exp const& l, Exp const& r ) override {
		throw Error("#unsupported");
	}
};

#endif
