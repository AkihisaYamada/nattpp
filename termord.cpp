#include "termord.hpp"
#include "poly.hpp"

using namespace std;

Smt::Compare PathOrder::compare( Exp const& l, Exp const& r ) {
	if( auto const& opt = _table.find({l,r}) ) {
//		DEB( l << " <=> " << r << " = " << *opt );
		return *opt;
	}
	auto some_ge = Smt::disj(l.args(),[&]( auto const& larg ){
		return compare(larg,r).ge;
	});
	if( some_ge == Smt::TRUE ) {
//		DEB( l << " <=> " << r << " = {true,true}" );
		return {Smt::TRUE,Smt::TRUE};
	}
	auto all_gt = Smt::conj(r.args(),[&]( auto const& rarg ){
		return compare(l,rarg).gt;
	});
	auto lf = l.fun();
	auto rf = r.fun();
	auto const& lprec = _precs.find(lf);
	auto const& rprec = _precs.find(rf);
	if( !lprec ) {// lhs is a variable
//		DEB( l << " <=> " << r << " = {" << (all_gt && lf == rf) << ", false}" );
		return { all_gt && lf == rf, Smt::FALSE };
	}
	some_ge = solver().let(Smt::BOOL,some_ge);
	if( !rprec || // rhs is a variable
		all_gt == Smt::FALSE
	) {
//		DEB( l << " <=> " << r << " = " << some_ge );
		return {some_ge,some_ge};
	}
	all_gt = solver().let(Smt::BOOL,all_gt);
	auto const& [args_ge,args_gt] = lex_compare<Term<std::string>>(
		[&]( auto const& x, auto const& y ){ return compare(x,y); },
		l.args(), r.args()
	);
	auto const& [pge,pgt] = Smt::compare(*lprec,*rprec);
	auto const& gt = solver().let( Smt::BOOL, some_ge || ( all_gt && ( pgt || ( pge && args_gt ) ) ) );
	auto const& ge = gt || ( all_gt && pge && args_ge );
	_table.insert(pair{l,r},Smt::Compare{ge,gt});
//	DEB( l << " <=> " << r << " = {" << ge << ", " << gt << '}' );
	return {ge,gt};
}

std::vector<size_t> order_some_rule( TrsOrder& order, Trs::Rules const& rules ) {
	vector<pair<size_t,Smt::PostExp>> gts;
	auto& solver = order.solver();
	if( solver.is_sat() || solver.is_unsat() ) {
		solver.pop();
	}
	solver.push();
	for( auto [i,rule] : rules ) {
		if( order.verbosity() & TermOrder::RULE ) {
			cerr << "; " << rule << endl;
		}
		auto const& [ge,gt] = order.order_rule(i);
		solver.ass(ge);
		gts.emplace_back(i,gt);
	}
	solver.ass( Smt::disj(gts,[]( auto const& gt ){ return gt.second; }) );
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

std::unique_ptr<TrsOrder> TrsOrder::of(
	Exp const& x,
	Trs::Sig const& sig,
	Trs::Rules const& trs,
	bool mono,
	std::function<Smt::Solver()> const& default_smt,
	Smt::Sort const& default_sort
) {
	auto const& f = x.fun();
	size_t n = 0;
	Opt<Exp> smt;
	auto mk_smt = [&]{ return smt ? Smt::Solver::of(*smt) : default_smt(); };
	Opt<Smt::Sort> sort;
	auto mk_sort = [&]{ return sort ? *sort : default_sort; };
	Verb verb = NONE;
	Exp::KeyValProc sort_key = [&]( string_view const& key, Exp const& val ){
		if( key == "sort" ) {
			sort = {Smt::Sort::of(val)};
			return true;
		}
		return false;
	};
	Exp::KeyValProc verb_key = [&]( string_view const& key, Exp const& val ){
		if( key == "verbosity" ) {
			if( val == "rule" ) {
				verb = RULE;
			} else {
				throw Error("#unknown-verbosity",val);
			}
			return true;
		}
		return false;
	};
	Exp::KeyValProc solver_key = [&]( string_view const& key, Exp const& val ){
		if( key == "smt" ) {
			smt = {val};
			return true;
		}
		return false;
	};
	if( f == "trivial" ) {
		x.process_keys(n,solver_key);
		return std::make_unique<TrivOrder>(mk_smt()); 
	} else if( f == "sum" ) {
		x.process_keys( n, sort_key || solver_key );
		return std::make_unique<DerivedTrsPosOrder<Poly>>
			( sig, trs, mono ? Poly::Template::MONO_SUM : Poly::Template::SUM, mk_smt(), mk_sort() );
	} else if( f == "template" ) {
		Poly::Template t = x.get_arg(n);
		x.process_keys( n, sort_key || solver_key );
		return std::make_unique<DerivedTrsPosOrder<Poly>>(sig,trs,t,mk_smt(),mk_sort());
	} else if( f == "path-order" ) {
		auto w = x.gets_arg(n);
		x.process_keys( n, w ? verb_key : solver_key || verb_key );
		x.get_end(n);
		return std::make_unique<PathOrder>(
			sig, trs,
			w ? of(*w,sig,trs,mono,default_smt,default_sort) : std::make_unique<TrivOrder>(mk_smt()),
			verb
		);
	} else {
		throw Error("#unknown-order",x);
	}
}

void TermOrder::test() {
	cout << "=== TermOrder ===" << endl;
	auto z3 = Smt::Z3(Smt::LIA);
	auto x = z3.declare_const("x",Smt::INT);
	auto y = z3.declare_const("y",Smt::INT);
	auto z = z3.declare_const("z",Smt::INT);
	cout << lex_compare<Smt::PostExp>(Smt::compare,{x,y},{x,z}).ge << endl;
	cout << lex_compare<Smt::PostExp>(Smt::compare,{x},{x,z}).ge << endl;

	Trs::Sig sig = {{"+",{2}}};
	Trs::Rules trs;
	trs.insert(0,Trs::Rule{{"+","x","y"},{"x"}});

	auto lpo = PathOrder(sig,trs,std::make_unique<TrivOrder>(Smt::Z3(Smt::LIA)),TermOrder::Verb::RULE);
	cout << lpo.order_rule(0).gt << endl;

}