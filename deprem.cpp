#include "deprem.hpp"

bool order_some_dp(
	TrsOrder& order,
	Trs::Rules const& rules,
	Dps const& dps,
	std::function<void(std::vector<size_t>&&)> const& f
) {
	auto& solver = order.solver();
	Smt::PostExp some_gt = false;
	Smt::PostExp all_ge = true;
	std::vector<std::pair<size_t,Smt::PostExp>> gts;
	for( auto const& [i,rule] : rules ) {
		auto const& [ge,gt] = order.order_rule(rule.first,rule.second,i);
		if( order.log() & TermOrder::RULE ) {
			std::cerr << "; " << rule << std::endl;
		}
		all_ge = all_ge && ge;
	}
	for( auto const& [i,dp] : dps.map ) {
		auto const& [ge,gt] = order.compare(dp.first,dp.second);
		all_ge = all_ge && ge;
		some_gt = some_gt || gt;
		gts.emplace_back(i,gt);
	}
	solver.push();
	solver.ass( all_ge && some_gt );
	solver.check_sat();
	if( solver.result().is_sat() ) {
		std::vector<size_t> rem;
		for( auto [i,gt] : gts ) {
			if( solver.get_value(gt) == Smt::TRUE ) {
				rem.push_back(i);
			}
		}
		f(std::move(rem));
		solver.pop();
		return true;
	}
	solver.pop();
	return false;
}
