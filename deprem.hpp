#ifndef DEPREM_HPP
#define DEPREM_HPP

#include "termord.hpp"
#include "dp.hpp"

std::vector<size_t> order_some_dp( TrsOrder& order, Trs::Rules const& rules, Dps const& dps );

#endif