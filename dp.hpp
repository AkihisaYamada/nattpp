#ifndef DP_HPP
#define DP_HPP

#include"trs.hpp"

struct Dp {
	Exp first;
	Exp second;
	size_t org;
	Pos rpos;
private:
	Printable _content;
public:
	Dp( Exp const& f, Exp const& s, size_t o, Pos p );
	Printable const& content() const& { return _content; }
};

using Dps = Map<size_t,Dp>;

Dps make_dps( Trs::Sig const& sig, Trs::Rules const& rules );

inline std::ostream& operator<<( std::ostream& os, Dp const& dp ) {
	return os << '(' << dp.content() << ')';
}

std::ostream& operator<<( std::ostream& os, Dps const& dps );

#endif
