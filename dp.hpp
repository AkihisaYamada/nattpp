#ifndef DP_HPP
#define DP_HPP

#include"trs.hpp"

struct Dp {
	Exp first;
	Exp second;
	size_t org;
	Pos rpos;
	std::ostream& print_content( std::ostream& os ) const&;
};

using Dps = Map<size_t,Dp>;

Dps make_dps( Trs::Sig const& sig, Trs::Rules const& rules );

inline std::ostream& operator<<( std::ostream& os, Dp const& dp ) {
	return dp.print_content( os << '(' ) << ')';
}

std::ostream& operator<<( std::ostream& os, Dps const& dps );

#endif
