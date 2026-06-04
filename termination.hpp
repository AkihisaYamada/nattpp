#ifndef TERMINATION_HPP_
#define TERMINATION_HPP_

#include<list>
#include"trs.hpp"
#include"poly.hpp"

class TrsAnnotator {
	
};

class TermOrder {
public:
	virtual Smt::PreExp operator()( Exp const& l, Exp const& r ) = 0;
};

class RuleRemover {
protected:
	TermOrder& order;
	Trs::Rules& rules;
	std::set<size_t>& used;
	std::vector<std::pair<Smt::PostExp,Smt::PostExp>> ords;
	Smt::Solver& solver;
	RuleRemover(
		TermOrder& order,
		Trs::Rules& rules,
		std::set<size_t>& used,
		Smt::Solver& solver
	);
public:
	std::vector<size_t> remove();
private:
	RuleRemover( RuleRemover const& ) = delete;
};

template<typename A>
class DerivedTermOrder : public TermOrder {
	Smt::Solver& solver;
	DerivedTermOrder( DerivedTermOrder const& ) = delete;
public:
	Algebra::Intp<std::string,A> const intp;
	Algebra::Deriver<std::string, typename A::Sig> const deriver;
	DerivedTermOrder(
		Trs::Sig const& sig,
		A::Template const& temp,
		Smt::Solver& solver,
		Smt::BaseSort const& sort
	) : solver(solver),
		deriver(temp.deriver(sig,solver)),
		intp(A::expand(deriver.derive(A::algebra(solver)),solver,sort)) {
	}
;
	Smt::PreExp operator()( Exp const& l, Exp const& r ) override {
		return A::order(intp.eval(l),intp.eval(r),solver);
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
	Smt::PreExp operator()( Exp const& l, Exp const& r ) override {
		throw Error("#unsupported");
	}
};

template<typename A>
class DerivedRuleRemover : public DerivedTermOrder<A>, public RuleRemover {
public:
	DerivedRuleRemover(
		Trs::Sig const& sig,
		Trs::Rules& rules,
		std::set<size_t>& used,
		Poly::Template const& temp,
		Smt::Solver& solver,
		Smt::BaseSort const& sort
	) : DerivedTermOrder<A>(sig,temp,solver,sort),
		RuleRemover(*this,rules,used,solver) {
	}
};

class DpProc {

};


#endif