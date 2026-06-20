#ifndef DEPREM_HPP
#define DEPREM_HPP

#include "termord.hpp"

bool order_some_rule(
	TrsOrder& order,
	Trs::Rules const& rules,
	std::function<void(std::vector<size_t>&&)> f
);

bool order_some_dp(
	TrsOrder& order,
	Trs::Rules const& rules,
	Trs::Rules const& dps,
	std::function<void(std::vector<size_t>&&)> const& f
);

#endif