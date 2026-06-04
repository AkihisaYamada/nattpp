#include "termination.hpp"
#include "poly.hpp"

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
	if( solver.is_sat() || solver.is_unsat() ) {
		solver.pop();
	}
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
	return ret;
}
