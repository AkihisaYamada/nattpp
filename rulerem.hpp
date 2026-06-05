#include "termord.hpp"

class RuleRemover : public TermOrder {
	std::unique_ptr<TermOrder> _ptr;
protected:
	Trs::Rules& rules;
	std::set<size_t>& used;
	std::vector<std::pair<Smt::PostExp,Smt::PostExp>> ords;
	Smt::Solver& solver;
public:
	RuleRemover(
		std::unique_ptr<TermOrder>&& ptr,
		Trs::Rules& rules,
		std::set<size_t>& used,
		Smt::Solver& solver
	);
	std::vector<size_t> remove();
	Smt::PreExp operator()( Exp const& l, Exp const& r ) override {
		return (*_ptr)(l,r);
	}
	std::ostream& print_name( std::ostream& os ) override {
		return _ptr->print_name(os);
	}
	std::ostream& print( std::ostream& os, Trs::Sig const& sig ) override {
		return _ptr->print(os,sig);
	}
};
