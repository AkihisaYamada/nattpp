#ifndef DEPREM_HPP
#define DEPREM_HPP

#include "termord.hpp"
#include "problem.hpp"

bool order_some_rule(
	TrsOrder& order,
	Trs::Rules const& rules,
	std::function<void(std::vector<ruleId>&&)> f
);

/**
 * @param callback(edges,usables) is called, where edges contain
 * - `{i,{}}` if DP i is oriented
 * - `{i,{j,...}}` if edges i->j ... are separated
 */
bool order_some_dp_or_edge(
	UsableRuleOrder& order,
	Problem const& problem,
	Problem::Component const& component,
	std::function<void(std::vector<std::pair<ruleId,std::vector<ruleId>>>&&,std::vector<ruleId>&&)> const& callback
);

#endif