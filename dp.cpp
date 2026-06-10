#include "dp.hpp"

using namespace std;

static void collect_dps(
	Trs::Sig const& sig, size_t org, Exp const& l, Exp const& r,
	Map<size_t,Dp>& dps, Pos& rpos, size_t depth, size_t& dp_ind
) {
	if( auto rank = sig.find(r.fun()) )
	if( rank->defined ) {
		dps.insert(dp_ind,Dp{l,r,org,rpos});
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
	for( auto [org,rule] : rules ) {
		collect_dps(sig,org,rule.first,rule.second,ret,pos,0,dp_ind);
	}
	return std::move(ret);
}

ostream& operator<<( ostream& os, Dp const& dp ) {
	return os << "(dp " << dp.first << ' ' << dp.second << " :origin " << dp.org << " :r-pos " << dp.rpos << ')';
}

ostream& operator<<( ostream& os, Dps const& rules ) {
	os << "(make-dps";
	for( auto const& [i,rule] : rules ) {
		os << "\n  " << rule << " :number " << i << flush;
	}
	return os << ')';
}
