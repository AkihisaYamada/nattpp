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
		auto const& [ge,gt] = order.rule_order(i,rule.first,rule.second);
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

static std::pair<Set<ruleId>,Smt::PostExp> _contain_usables( UsableRuleOrder& order, Problem const& p ){
	Set<ruleId> usables;// collect potential usable rules
	Smt::PostExp ret = true;
	for( auto const& [i,js] : p.components.front().graph ) {
		if( auto const& us = p.dp_use_map.find(i) ) {
			for( auto const& u : **us ) {
				usables.emplace(u);
			}
		}
	}
	for( auto const& i : usables ) {
		if( auto const& rule = p.rules.find(i) ) {
			auto const& [l,r,w] = *rule;
			auto const& [ge,str] = order.rule_order(i,l,r);
			ret = ret && order.rule_used(i).imp(ge);// if the rule is used, then it should be weakly oriented
		}
	}
	return {usables,ret};
}

bool order_some_dp(
	UsableRuleOrder& order,
	Problem const& p,
	std::function<void(std::vector<ruleId>&&,std::vector<ruleId>&&)> const& callback
) {
	auto& solver = order.solver();
	auto& component = p.components.front();
	auto [usables,contain_usables] = _contain_usables(order,p);
	Smt::PostExp orient_some_dp = false, contain_dps = true;
	std::vector<std::pair<ruleId,Smt::PostExp>> encs;
	for( auto const& [i,js] : component.graph ) {
		auto const& [l,r,w] = *ASSERTED(p.rules.find(i));
		auto const& [ge,str] = order.rule_order(i,l,r);
		contain_dps = contain_dps && ge && order.term_used(r);// rhs of DP should be marked used
		orient_some_dp = orient_some_dp || str;
		encs.emplace_back(i,str);
	}
	bool ret = solver.push().ass( contain_usables && contain_dps && orient_some_dp ).check_sat().result().is_sat();
	if( ret ) {
		std::vector<ruleId> dp_results;
		for( auto& [i,str] : encs ) {
			if( solver.get_value(str) == true ) {
				dp_results.emplace_back(i);
			}
		}
		std::vector<ruleId> uses;
		for( auto const& usable : usables ) {
			if( solver.get_value(order.rule_used(usable)) == true ) {
				uses.push_back(usable);
			}
		}
		callback(std::move(dp_results),std::move(uses));
	}
	solver.pop();
	return ret;
}

bool unorder_some_edge(
	UsableRuleOrder& order,
	Problem const& p,
	std::function<void(std::vector<std::pair<ruleId,std::vector<ruleId>>>&&,std::vector<ruleId>&&)> const& callback
) {
	auto& solver = order.solver();
	auto& component = p.components.front();
	Smt::PostExp separate_some_edge = false;
	auto [usables,contain_usables] = _contain_usables(order,p);
	Smt::PostExp mark_usables = true;
	struct OrientInfo {
		ruleId i;
		std::vector<std::pair<ruleId,Smt::PostExp>> edges;
	};
	std::vector<OrientInfo> encs;

	for( auto const& [i,js] : component.graph ) {
		auto const& [l,r,w] = *ASSERTED(p.rules.find(i));
		for( auto const& t : r.args() ) {
			mark_usables = mark_usables && order.term_used(t);// args of DP should be marked used
		}
		auto& edges = encs.emplace_back(i).edges;
		for( auto const& j : js ) {
			auto sep = order.edge_co_order(i,j,p);
			if( sep == false ) continue;
			separate_some_edge = separate_some_edge || sep;
			edges.emplace_back(j,sep);
		}
	}
	bool ret = solver.push().ass( contain_usables && mark_usables && separate_some_edge ).check_sat().result().is_sat();
	if( ret ) {
		std::vector<std::pair<ruleId,std::vector<ruleId>>> dp_results;
		for( auto& [i,edge_encs] : encs ) {
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
		std::vector<ruleId> uses;
		for( auto const& usable : usables ) {
			if( solver.get_value(order.rule_used(usable)) == true ) {
				uses.push_back(usable);
			}
		}
		callback(std::move(dp_results),std::move(uses));
	}
	solver.pop();
	return ret;
}

bool bound_by_edge(
	UsableRuleOrder& order,
	Problem const& problem,
	std::function<void(std::vector<std::pair<ruleId,std::vector<ruleId>>>&&,std::vector<ruleId>&&)> const& callback
) {
	auto& solver = order.solver();
	auto& component = problem.components.front();
	Smt::PostExp rm_some_edge = false;
	auto [usables,contain_usables] = _contain_usables(order,problem);
	Smt::PostExp contain_dps = true;
	struct OrientInfo {
		ruleId i;
		Smt::PostExp str;
		std::vector<std::pair<ruleId,Smt::PostExp>> edges;
	};
	std::vector<OrientInfo> encs;

	for( auto const& [i,js] : component.graph ) {
		auto const& [l,r,w] = *ASSERTED(problem.rules.find(i));
		auto const& [ge,str] = order.rule_order(i,l,r);
		contain_dps = contain_dps && ge && order.term_used(r);// rhs of DP should be marked used
		auto& edges = encs.emplace_back(i,str).edges;
		Smt::PostExp node_cond = false;
		for( auto const& j : js ) {
			auto const& [s,t,v] = *ASSERTED(problem.rules.find(j));
			auto sep = order.separate_edge(i,j,problem);
			if( sep == false ) continue;
			node_cond = node_cond || sep;
			edges.emplace_back(j,sep);
		}
		rm_some_edge = rm_some_edge || node_cond;
	}
	bool ret = solver.push().ass( contain_usables && contain_dps && rm_some_edge ).check_sat().result().is_sat();
	if( ret ) {
		std::vector<std::pair<ruleId,std::vector<ruleId>>> dp_results;
		for( auto& [i,str,edge_encs] : encs ) {
			if( solver.get_value(str) == true ) {
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
		callback(std::move(dp_results),std::move(uses));
	}
	solver.pop();
	return ret;
}
