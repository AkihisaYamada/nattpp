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
 * @param callback is called on (oriented dps, usable rules)
 */
bool order_some_dp(
	UsableRuleOrder& order,
	Problem const& problem,
	std::function<void(std::vector<ruleId>&&,std::vector<ruleId>&&)> const& callback
);

/** remove edge by co-rewrite pair
 */
bool unorder_some_edge(
	UsableRuleOrder& order,
	Problem const& problem,
	std::function<void(std::vector<std::pair<ruleId,std::vector<ruleId>>>&&,std::vector<ruleId>&&)> const& callback
);

#endif