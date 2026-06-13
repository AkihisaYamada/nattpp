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
		auto const& l = rule.first;
		if( l.unapplied() && [&]( string const& v ){ return !sig.find(v); } ) {
			cerr << "(var-lhs " << org << ')' << endl;
			throw Answer::NO;
		}
		collect_dps(sig,org,l,rule.second,ret,pos,0,dp_ind);
	}
	return std::move(ret);
}

std::ostream& Dp::print_content( ostream& os ) const & {
	return os << first << ' ' << second << " :origin " << org << " :r-pos " << rpos;
}

ostream& operator<<( ostream& os, Dps const& dps ) {
	os << "(make-dps";
	for( auto const& [i,dp] : dps ) {
		os << "\n  (dp-n " << i << ' ' << dp.print_content() << ')' << flush;
	}
	return os << ')';
}
