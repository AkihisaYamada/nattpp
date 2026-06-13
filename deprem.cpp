#include "deprem.hpp"

std::vector<size_t> order_some_dp( TrsOrder& order, Trs::Rules const& rules, Dps const& dps ) {
	Smt::PostExp disj = Smt::FALSE;
	std::vector<std::pair<size_t,Smt::PostExp>> gts;
	auto& solver = order.solver();
	if( solver.is_sat() || solver.is_unsat() ) {
		solver.pop();
	}
	solver.push();
	for( auto const& [i,rule] : rules ) {
		auto const& [ge,gt] = order.order_rule(i);
		if( order.log() & TermOrder::RULE ) {
			std::cerr << "; " << rule << std::endl;
		}
		solver.ass(ge);
	}
	for( auto const& [i,dp] : dps ) {
		auto const& [ge,gt] = order.compare(dp.first,dp.second);
		solver.ass(ge);
		disj = disj || gt;
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
