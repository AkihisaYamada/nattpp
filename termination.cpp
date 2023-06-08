#include"termination.hpp"
using namespace std;

RuleRemover::RuleRemover(
	TermOrder& order,
	Trs::Rules& rules,
	std::set<size_t>& used,
	Smt::Solver& solver
) : order(order), rules(rules), used(used), ords(rules.size()), solver(solver) {
	for( size_t i : used ) {
		auto const& ord = solver.expand(
			Smt::Let( (Smt::BOOL,Smt::BOOL), order(rules[i].first,rules[i].second) ) ^ []( Smt::PreExp const& val ){
				return val;
			}
		);
		ords[i] = { Smt::car(ord), Smt::cdr(ord) };
	}
}

std::vector<size_t> RuleRemover::remove() {
	Smt::PostExp conj = Smt::TRUE;
	Smt::PostExp disj = Smt::FALSE;
	solver.push();
	for( size_t i : used ) {
		conj.conj_eq(ords[i].first);
		disj.disj_eq(ords[i].second);
	}
	solver.ass(conj);
	solver.ass(disj);
	solver.check_sat();
	std::vector<size_t> ret;
	if( solver.result().is_sat() ) {
		for( auto it = used.begin(); it != used.end(); ) {
			if( solver.get_value(ords[*it].second) == Smt::TRUE ) {
				ret.push_back(*it);
				it = used.erase(it);
			} else {
				it++;
			}
		}
	}
	solver.pop();
	return ret;
}
DerivedTermOrder::DerivedTermOrder(
	Trs::Sig const& sig,
	Poly::Template const& temp,
	Smt::Solver& solver,
	Smt::BaseSort const& sort
) : solver(solver),
	deriver(temp.deriver(sig,solver)),
	intp(deriver.derive(Poly::algebra(solver,sort))) {
}

DerivedRuleRemover::DerivedRuleRemover(
	Trs::Sig const& sig,
	Trs::Rules& rules,
	std::set<size_t>& used,
	Poly::Template const& temp,
	Smt::Solver& solver,
	Smt::BaseSort const& sort
) : DerivedTermOrder(sig,temp,solver,sort),
	RuleRemover(*this,rules,used,solver) {
}
