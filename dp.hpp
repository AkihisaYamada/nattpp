#ifndef DP_HPP
#define DP_HPP

#include"problem.hpp"

struct Dp {
	Exp first;
	Exp second;
	size_t org;
	Pos rpos;
};

using Dps = Map<size_t,Dp>;

Dps make_dps( Trs::Sig const& sig, Trs::Rules const& rules );

std::ostream& operator<<( std::ostream& os, Dp const& dp );

std::ostream& operator<<( std::ostream& os, Dps const& dps );

#endif
