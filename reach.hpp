#ifndef REACH_HPP
#define REACH_HPP
#include "trs.hpp"

bool may_reach( Trs const& trs, Exp const& s, Exp const& t, size_t fuel, bool root );

#endif
