#include "termord.hpp"

using namespace std;

template<typename T>
Smt::Compare lex_compare(
	std::function<Smt::Compare(T const&, T const&)> const& comp, std::vector<T> const& ls, std::vector<T> const& rs
) {
	auto all_ge = Smt::TRUE, gt = Smt::FALSE;
	auto ln = ls.size();
	auto rn = rs.size();
	for( size_t i = 0;; i++ ) {
		if( i == ln ) {
			return { gt || all_ge, i == rn ? gt : Smt::FALSE };
		} else if( i == rn ) {
			return { gt || all_ge, gt || all_ge };
		}
		auto const& c = comp(ls[i],rs[i]);
		gt |= all_ge && c.gt;
		all_ge &= c.ge;
	}
}

Smt::Compare PathOrder::compare( Exp const& l, Exp const& r ) {
	if( auto const& opt = _table.find({l,r}) ) return *opt;
	auto some_ge = Smt::FALSE;
	for( auto const& larg : l.args() ) {
		auto const& [lge,lgt] = compare(larg,r);
		some_ge |= lge;
	}
	if( some_ge == Smt::TRUE ) return {Smt::TRUE,Smt::TRUE};
	auto all_gt = Smt::TRUE;
	for( auto const& rarg : r.args() ) {
		auto const& [rge,rgt] = compare(l,rarg);
		all_gt &= rgt;
	}
	auto const& lf = _sig.find(l.fun());
	auto const& rf = _sig.find(r.fun());
	if( all_gt == Smt::FALSE || !lf || !rf ) return {some_ge,some_ge};
	auto const& lprec = lf->prec;
	auto const& rprec = rf->prec;
	auto const& arg_ord = lex_compare<Term<std::string>>(
		[&]( auto const& x, auto const& y ){ return compare(x,y); },
		l.args(), r.args()
	);
	auto const& ge = some_ge || all_gt && Smt::ge(lprec,rprec) && arg_ord.ge;
	auto const& gt = some_ge || all_gt && ( Smt::gt(lprec,rprec) || Smt::ge(lprec,rprec) && arg_ord.gt);
	return {ge,gt};
}

std::vector<size_t> order_some_rule( TrsOrder& order, Trs::Rules const& rules ) {
	Smt::PostExp disj = Smt::FALSE;
	vector<pair<size_t,Smt::PostExp>> gts;
	auto& solver = order.solver();
	if( solver.is_sat() || solver.is_unsat() ) {
		solver.pop();
	}
	solver.push();
	for( auto [i,rule] : rules ) {
		auto const& [ge,gt] = order.order_rule(i);
		solver.ass(ge);
		disj |= gt;
		gts.emplace_back(i,gt);
	}
	solver.ass(disj);
	solver.check_sat();
	std::vector<size_t> ret;
	if( solver.result().is_sat() ) {
		for( auto [i,gt] : gts ) {
			if( solver.get_value(gt) == Smt::TRUE ) {
				ret.push_back(i);
			}
		}
	}
	return std::move(ret);
}

std::unique_ptr<TrsOrder> TrsOrder::of( Exp const& x ) {
	
}
