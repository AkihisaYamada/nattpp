#include "dp.hpp"

using namespace std;

std::ostream& Dp::print_content( ostream& os ) const & {
	return os << first << ' ' << second << " :origin " << org << " :r-pos " << rpos;
}

ostream& operator<<( ostream& os, Dps const& dps ) {
	os << "(make-dps";
	for( auto const& [i,dp] : dps.map ) {
		os << "\n  (dp-n " << i << ' ' << dp.print_content() << ')' << flush;
	}
	return os << ')';
}
