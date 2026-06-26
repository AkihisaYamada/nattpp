#include "deprem.hpp"

using namespace std;

bool order_some_rule(
	TrsOrder& order,
	Trs::Rules const& rules,
	std::function<void(std::vector<size_t>&&)> f
) {
	auto& solver = order.solver();
	vector<pair<size_t,Smt::PostExp>> gts;
	Smt::PostExp all_ge = true;
	for( auto const& [i,rule] : rules ) {
		if( order.log() & TermOrder::RULE ) {
			cerr << "; " << rule << endl;
		}
		auto const& [ge,gt] = order.rule_compare(i,rule.first,rule.second);
		all_ge = all_ge && ge;
		gts.emplace_back(i,gt);
	}
	solver.push();
	solver.ass( order.mono() && all_ge && Smt::disj(gts,[]( auto const& gt ){ return gt.second; }) );
	solver.check_sat();
	if( solver.result().is_sat() ) {
		std::vector<size_t> ret;
		for( auto [i,gt] : gts ) {
			if( solver.get_value(gt) == Smt::TRUE ) {
				ret.push_back(i);
			}
		}
		f(std::move(ret));
		solver.pop();
		return true;
	}
	solver.pop();
	return false;
}

bool order_some_dp(
	UsableRuleOrder& order,
	Problem const& p,
	Trs::Rules const& dps,
	std::function<void(std::vector<size_t>&&,Set<size_t>const&)> const& f
) {
	auto& solver = order.solver();
	Smt::PostExp some_gt = false;
	Smt::PostExp all_ge = true;
	std::vector<std::pair<size_t,Smt::PostExp>> gts;
	Set<size_t> usables;// collect potential usable rules
	for( auto const& [i,rule] : dps ) {
		if( auto const& us = p.usable_graph.find(i) ) {
			for( auto const& u : **us ) {
				usables.emplace(u);
			}
		}
	}
	for( auto const& i : usables ) {
		if( auto const& rule = p.main.rules.find(i) ) {
			auto const& [l,r,w] = *rule;
			auto const& [ge,gt] = order.rule_compare(i,l,r);
			all_ge = all_ge && order.rule_used(i).imp(ge);// if the rule is used, then it should be weakly oriented
		}
	}
	for( auto const& [i,dp] : dps ) {
		auto const& [l,r,w] = dp;
		auto const& [ge,gt] = order.rule_compare(i,l,r);
		all_ge = all_ge && ge && order.term_used(r);// rhs of DP should be marked used
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
		f(std::move(rem),usables);
		solver.pop();
		return true;
	}
	solver.pop();
	return false;
}
