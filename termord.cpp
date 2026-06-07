#include "termord.hpp"

using namespace std;


std::vector<size_t> order_some_rule( TrsOrder& order, Trs::Rules const& rules ) {
	Smt::PostExp disj = Smt::FALSE;
	vector<pair<size_t,Smt::PostExp>> gts;
	auto& solver = order.solver();
	if( solver.is_sat() || solver.is_unsat() ) {
		solver.pop();
	}
	solver.push();
	for( auto [i,rule] : rules ) {
		auto const& [ge,gt] = order.order_rule(i);
		solver.ass(ge);
		disj |= gt;
		gts.emplace_back(i,gt);
	}
	solver.ass(disj);
	solver.check_sat();
	std::vector<size_t> ret;
	if( solver.result().is_sat() ) {
		for( auto [i,gt] : gts ) {
			if( solver.get_value(gt) == Smt::TRUE ) {
				ret.push_back(i);
			}
		}
	}
	return std::move(ret);
}
