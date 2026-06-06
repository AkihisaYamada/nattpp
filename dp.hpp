#ifndef DP_HPP
#define DP_HPP

#include"problem.hpp"

struct Dp {
	Trs::Term first;
	Trs::Term second;
	size_t org;
	Pos rpos;
};

Map<size_t,Dp> make_dps( Trs::Sig const& sig, Trs::Rules const& rules );

std::ostream& operator<<( std::ostream& os, Dp const& dp );

std::ostream& operator<<( std::ostream& os, Map<size_t,Dp> const& dps );

#endif
