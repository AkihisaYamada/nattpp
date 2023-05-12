#ifndef TERMINATION_HPP_
#define TERMINATION_HPP_

#include<list>
#include"trs.hpp"
#include"template.hpp"
#include"poly.hpp"

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

class DerivedTermOrder : public TermOrder {
	Deriver::Map deriver;
	Algebra::Intp<Poly> intp;
	DerivedTermOrder( DerivedTermOrder const& ) = delete;
public:
	DerivedTermOrder( Trs::Sig const& sig, Template const& temp, Smt::Solver& solver );
	Smt::PreExp operator()( Exp const& l, Exp const& r ) override {
		return intp.eval(l).order(intp.eval(r));
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
	}
};

class DerivedRuleRemover : public DerivedTermOrder, public RuleRemover {
public:
	DerivedRuleRemover( Trs::Sig const& sig, Trs::Rules& rules, std::set<size_t>& used, Template const& temp, Smt::Solver& solver );
};

class DpProc {

};


#endif