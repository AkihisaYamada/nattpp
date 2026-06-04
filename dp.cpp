#include<set>
#include "dp.hpp"

using namespace std;

static void take_deps( Trs::Sig const& sig, Exp const& r, set<Pos>& deps, Pos& pos ) {
	if( auto rank = sig.find(r.fun()) )
	if( rank->defined ) {
		deps.insert(pos);
	}
	auto i = pos.emplace_back(0);
	for( auto a : r.args() ) {
		take_deps(sig,a,deps,pos);
		i++;
	}
	pos.pop_back();
}
DepMap dep_map( Trs::Sig const& sig, Trs::Rules const& rules ) {
	DepMap ret;
	for( auto const& rule : rules ) {
		auto deps = ret.emplace_back();
		Pos pos;
		take_deps(sig,rule.second,deps,pos);
	}
}
