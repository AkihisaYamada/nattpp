#include "dp.hpp"

using namespace std;

static void collect_dps(
	Trs::Sig const& sig, size_t org, Exp const& l, Exp const& r,
	Map<size_t,Dp>& dps, Pos& rpos, size_t depth, size_t& dp_ind
) {
	if( auto rank = sig.find(r.fun()) )
		if( rank->defined ) {
			dps.emplace(dp_ind,Dp{l,r,org,rpos});
			dp_ind++;
		}
	rpos.emplace_back(0);
	for( auto const& a : r.args() ) {
		collect_dps(sig,org,l,a,dps,rpos,depth+1,dp_ind);
		rpos[depth]++;
	}
	rpos.pop_back();
}
Dps make_dps( Trs::Sig const& sig, Trs::Rules const& rules ) {
	Dps ret;
	Pos pos;
	size_t dp_ind = 0;
	for( auto const& [org,rule] : rules ) {
		collect_dps(sig,org,rule.first,rule.second,ret,pos,0,dp_ind);
	}
	return std::move(ret);
}

Dp::Dp( Exp const& f, Exp const& s, size_t o, Pos p ) : first(f), second(s), org(o), rpos(p),
	_content([this]( ostream& os )->ostream&{
		return os << first << ' ' << second << " :origin " << org << " :r-pos " << rpos;
	}) {}

ostream& operator<<( ostream& os, Dps const& dps ) {
	os << "(make-dps";
	for( auto const& [i,dp] : dps ) {
		os << "\n  (dp " << dp.content() << " :number " << i << ')' << flush;
	}
	return os << ')';
}
