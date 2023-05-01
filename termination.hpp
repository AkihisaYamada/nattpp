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

class DerivedRuleRemover : public DerivedTermOrder, public RuleRemover {
public:
	DerivedRuleRemover( Trs::Sig const& sig, Trs::Rules& rules, std::set<size_t>& used, Template const& temp, Smt::Solver& solver );
};

class DpProc {

};


#endif