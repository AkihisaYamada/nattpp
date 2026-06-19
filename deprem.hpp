#ifndef DEPREM_HPP
#define DEPREM_HPP

#include "termord.hpp"
#include "dp.hpp"

bool order_some_dp(
	TrsOrder& order,
	Trs::Rules const& rules,
	Dps const& dps,
	std::function<void(std::vector<size_t>&&)> const& f
);

#endif