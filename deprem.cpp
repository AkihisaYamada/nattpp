#include "deprem.hpp"

using namespace std;

bool order_some_rule(
	TrsOrder& order,
	Trs::Rules const& rules,
	std::function<void(std::vector<ruleId>&&)> f
) {
	auto& solver = order.solver();
	vector<pair<ruleId,Smt::PostExp>> gts;
	Smt::PostExp all_ge = true;
	for( auto const& [i,rule] : rules ) {
		if( order.log() & RULE ) {
			cerr << "; " << rule << endl;
		}
		auto const& [ge,gt] = order.rule_compare(i,rule.first,rule.second);
		all_ge = all_ge && ge;
		gts.emplace_back(i,gt);
	}
	solver.push();
	solver.ass( order.mono() && all_ge && Smt::PostExp::disj(gts,[]( auto const& gt ){ return gt.second; }) );
	solver.check_sat();
	if( solver.result().is_sat() ) {
		std::vector<ruleId> ret;
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

bool order_some_dp_or_edge(
	UsableRuleOrder& order,
	Problem const& p,
	Problem::Component const& component,
	std::function<void(std::vector<std::pair<ruleId,std::vector<ruleId>>>&&,std::vector<ruleId>&&)> const& f
) {
	auto& solver = order.solver();
	Smt::PostExp orient_some_dp = false;
	Smt::PostExp separate_some_edge = false;
	Smt::PostExp contain_usables = true;
	Smt::PostExp contain_dps = true;
	struct OrientInfo {
		ruleId i;
		Smt::PostExp oriented;
		std::vector<std::pair<ruleId,Smt::PostExp>> edges;
	};
	std::vector<OrientInfo> encs;
	Set<ruleId> usables;// collect potential usable rules
	for( auto const& [i,js] : component.graph ) {
		if( auto const& us = p.usable_graph.find(i) ) {
			for( auto const& u : **us ) {
				usables.emplace(u);
			}
		}
	}
	for( auto const& i : usables ) {
		if( auto const& rule = p.rules.find(i) ) {
			auto const& [l,r,w] = *rule;
			auto const& [ge,str] = order.rule_compare(i,l,r);
			contain_usables = contain_usables && order.rule_used(i).imp(ge);// if the rule is used, then it should be weakly oriented
		}
	}
	for( auto const& [i,js] : component.graph ) {
		auto const& [l,r,w] = *ASSERTED(p.rules.find(i));
		auto const& [ge,str] = order.rule_compare(i,l,r);
		contain_dps = contain_dps && ge && order.term_used(r);// rhs of DP should be marked used
		orient_some_dp = orient_some_dp || str;
		auto& edges = encs.emplace_back(i,str).edges;
		// constraints for edge removal
		for( auto const& j : js ) {
			auto sep = order.edge_separate(i,j,p);
			if( sep == false ) continue;
			separate_some_edge = separate_some_edge || sep;
			edges.emplace_back(j,sep);
		}
	}
	solver.push();
	solver.ass( contain_usables && contain_dps && (orient_some_dp || separate_some_edge) );
	solver.check_sat();
	if( solver.result().is_sat() ) {
		std::vector<std::pair<ruleId,std::vector<ruleId>>> dp_results;
		for( auto& [i,str,edge_encs] : encs ) {
			if( solver.get_value(str) == true ) {
				dp_results.emplace_back(i,std::vector<ruleId>{});
			} else {// i is not oriented, check which edges are separated
				std::vector<ruleId> edge_results;
				for( auto const& [j,sep] : edge_encs ) {
					if( solver.get_value(sep) == true ) {
						edge_results.emplace_back(j);
					}
				}
				if( !edge_results.empty() ) {// some edges are separated
					dp_results.emplace_back(i,std::move(edge_results));
				}
			}
		}
		std::vector<ruleId> uses;
		for( auto const& usable : usables ) {
			if( solver.get_value(order.rule_used(usable)) == true ) {
				uses.push_back(usable);
			}
		}
		f(std::move(dp_results),std::move(uses));
		solver.pop();
		return true;
	}
	solver.pop();
	return false;
}
