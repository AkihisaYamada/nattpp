#include<set>
#include "util.hpp"
#include "dp.hpp"

using namespace std;

static void take_deps( Trs::Sig const& sig, Exp const& r, set<Pos>& deps, Pos& pos ) {
DEB(pos);
	if( auto rank = sig.find(r.fun()) )
	if( rank->defined ) {
		deps.insert(pos);
	}
	auto& i = pos.emplace_back(0);
	for( auto const& a : r.args() ) {
		take_deps(sig,a,deps,pos);
		i++;
	}
	pos.pop_back();
}
Dp::Rule::Rule( Trs::Rule const& rule, Trs::Sig const& sig ) : Trs::Rule(rule) {
	Pos pos;
	take_deps(sig,rule.second,deps,pos);
}

ostream& operator<<( ostream& os, Dp::Rule const& rule ) {
	rule.print_contents( os << "(rule " ) << " :depends (";
	print_list(os,rule.deps.begin(),rule.deps.end(),[]( Pos const& pos ){ return pos; });
	return os << "))";
}

ostream& operator<<( ostream& os, Dp::Rules const& rules ) {
	os << "(dp";
	for( auto const& rule : rules ) {
		os << "\n  " << rule << flush;
	}
	return os << ')';
}
