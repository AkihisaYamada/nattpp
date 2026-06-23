#include"reach.hpp"

bool may_reach( Trs const& trs, Exp const& s, Exp const& t, size_t fuel ) {
	auto const& [f,ss] = *s;
	auto const& frank = trs.sig.find(f);
	if( !frank ) return true;// x ↠ _
	auto const& [g,ts] = *t;
	auto const& grank = trs.sig.find(g);
	if( !grank ) return true;// _ ↠ y
	if( f == g ) {// f(...) ↠ f(...) if all arguments may reach
		for( auto sit = ss.begin(), tit = ts.begin(); ; sit++, tit++ ) {
			if( sit == ss.end() ) {
				if( tit == ts.end() ) return true;
				break;
			}
			if( tit == ts.end() ) break;
			if( !may_reach(trs,*sit,*tit,fuel) ) break;
		}
	}
	// otherwise, a root rewrite step must be involved
	if( fuel == 0 ) return true;
	fuel--;
	for( auto const& i : frank->defined_by ) {
		auto const& [l,r,w] = *ASSERTED(trs.rules.find(i));
		if( may_reach(trs,s,l,fuel) && may_reach(trs,r,t,fuel) ) return true;
	}
	return false;
}
