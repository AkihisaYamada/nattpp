#include "dp.hpp"

using namespace std;

static void collect_dps(
	Trs::Sig const& sig, size_t org, Exp const& l, Exp const& r,
	Dps& dps, Pos& rpos, size_t depth
) {
	if( auto rank = sig.find(r.fun()) )
		if( rank->defined ) {
			dps.emplace(l,r,org,rpos);
		}
	rpos.emplace_back(0);
	for( auto const& a : r.args() ) {
		collect_dps(sig,org,l,a,dps,rpos,depth+1);
		rpos[depth]++;
	}
	rpos.pop_back();
}
Dps make_dps( Trs::Sig const& sig, Trs::Rules const& rules, size_t max_ind ) {
	Dps ret{max_ind};
	Pos pos;
	for( auto const& [org,rule] : rules ) {
		auto const& l = rule.first;
		if( l.unapplied() && [&]( string const& v ){ return !sig.find(v); } ) {
			cerr << "(var-lhs " << org << ')' << endl;
			throw Answer::NO;
		}
		collect_dps(sig,org,l,rule.second,ret,pos,0);
	}
	return std::move(ret);
}
string mark_sym( string const& sym ) {
	return string("#")+sym;
}
Dp mark_dp( Trs::Sig const& sig, Trs::Sig& extra_sig, Dp const& dp ) {
	auto const& l = dp.first, &r = dp.second;
	auto lfm = mark_sym(l.fun()), rfm = mark_sym(r.fun());
	if( !extra_sig.find(lfm) ) {
		extra_sig.emplace(lfm,*ASSERTED(sig.find(l.fun())));
	}
	if( !extra_sig.find(rfm) ) {
		extra_sig.emplace(rfm,*ASSERTED(sig.find(r.fun())));
	}
	return Dp(app(lfm,l.args()),app(rfm,r.args()),dp.org,dp.rpos);
}
void mark_dps( Trs::Sig const& sig, Dps& dps ) {
	Dps temp{dps.max_index};
	for( auto const& [i,dp] : dps.map ) {
		temp.emplace(mark_dp(sig,temp.extra_sig,dp));
	}
	dps = std::move(temp);
}

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
