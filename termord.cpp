#include "termord.hpp"
#include "exp.hpp"
#include "poly.hpp"

using namespace std;

PathOrder::PathOrder(
	Trs::Sig const& sig,
	Trs::Rules const& rules,
	std::unique_ptr<TermOrder>&& weight,
	std::function<Status(Trs::Rank const&)> status,
	int log
) : _weight(std::move(weight)), _log(log) {
	if( log & DEBUG ) cerr << "; initializing path order" << endl;
	size_t sigsize = sig.size();
	auto& sol = solver();
	auto const& sort = sol.logic().base_sort();
	for( auto const&[f,rank] : sig ) {
		if( log & DEBUG ) cerr << "; fun " << f << ' ' << rank << endl;
		auto [info,suc] = _info.emplace(f,_SymInfo{});
		assert(suc);
		info.prec = sol.declare_fresh(sort);
		sol.ass( Smt::ge(info.prec,0) );
		if( log & DEBUG ) cerr << ";  prec: " << info.prec << endl;
		if( auto post_arity = status(rank).post_arity() ) {
			info.post_arity = *post_arity;
			for( size_t i = 0; i < rank.arity; i++ ) {
				auto& map = info.map.emplace_back();
				for( size_t k = 0; k < info.post_arity; k++ ) {// k-th place after mapping
					auto const& ik = map.emplace_back(sol.declare_fresh(Smt::BOOL));
					for( size_t j = 0; j < i; j++ ) {// k-th place cannot be shared
						sol.ass( !info.map[i][k] || !info.map[j][k] );
					}
				}
				// mapped[i] means i-th argument survives mapping
				info.mapped.emplace_back( sol.let(Smt::BOOL,Smt::disj(map)) );
				if( log & DEBUG ) cerr << ";  map[" << i << "] = " << print_list(map) << std::endl;
			}
		} else {
			for( size_t i = 0; i < rank.arity; i++ ) {// every argument survives
				info.mapped.push_back(true);
			}
		}
	}
	for( auto const& [n,rule] : rules ) {
		auto const& l = rule.first;
		auto const& r = rule.second;
		_ord.emplace(n,compare(l,r));
	}
}

Smt::Compare PathOrder::compare( Exp const& l, Exp const& r ) {
	if( auto const& opt = _table.find({l,r}) ) {
		return *opt;
	}
	auto memo = [&]( Smt::Compare const& comp ){
		if( log() & PAIR ) {
			cerr << "; " << l << " <=> " << r << " = " << comp << endl;
		}
		_table.emplace(pair{l,r},comp);
		return comp;
	};
	auto const& [lf,largs] = *l;
	auto const& linfo = _info.find(lf);
	auto some_arg_ge = (bool)linfo && Smt::disj( 0, largs.size(), [&]( size_t i ){
		return linfo->mapped[i] && compare(largs[i],r).ge;// l_i survives and l_i >= r
	} );
	if( some_arg_ge == Smt::TRUE ) {
		return memo({Smt::TRUE,Smt::TRUE});
	}
	auto const& [rf,rargs] = *r;
	auto const& rinfo = _info.find(rf);
	auto gt_all_arg = !rinfo || Smt::conj( 0, rargs.size(), [&]( size_t j ){
		return rinfo->mapped[j].imp( compare(l,rargs[j]).gt );// if r_j survives, then l > r_j
	});
	if( !linfo ) {// lhs is a variable
		return memo({ solver().let( Smt::BOOL, gt_all_arg && lf == rf ), Smt::FALSE });
	}
	some_arg_ge = solver().let(Smt::BOOL,some_arg_ge);
	if( !rinfo || // rhs is a variable
		gt_all_arg == Smt::FALSE
	) {
		return memo({some_arg_ge,some_arg_ge});
	}
	gt_all_arg = solver().let(Smt::BOOL,gt_all_arg);
	auto const& [args_ge,args_gt] = mapped_lex_compare(
		[&]( auto const& x, auto const& y ){ return compare(x,y); },
		linfo->post_arity, rinfo->post_arity, linfo->map, rinfo->map, largs, rargs
	);
	if( _log & DEBUG ) {
		cerr << "; [" << print_list(largs) << "] <=> [" << print_list(rargs) << "] = {" << args_ge << ", " << args_gt << '}' << endl;
	}
	auto const& [pge,pgt] = Smt::compare(linfo->prec,rinfo->prec);
	auto const& gt = solver().let( Smt::BOOL, some_arg_ge || ( gt_all_arg && ( pgt || ( pge && args_gt ) ) ) );
	auto const& ge = solver().let( Smt::BOOL, gt || ( gt_all_arg && pge && args_ge ) );
	return memo({ge,gt});
}

std::ostream& PathOrder::print_sym_info( std::ostream& os, std::string const& sym ) {
	auto info = _info.find(sym);
	auto ar = info->map.size();
	assert(info);
	os << "(prec " << solver().get_value(info->prec) << ')';
	if( info->post_arity > 0 ) {
		os << " (map ";
		auto f = [&]( size_t k ){
			size_t i = 0;
			for(;;){
				if( i == ar ) {
					os << '-';
					break;
				}
				if( solver().get_value(info->map[i][k]) == Smt::TRUE ) {
					os << i;
					break;
				}
				i++;
			}
		};
		f(0);
		for( size_t k = 1; k < info->post_arity; k++ ) {
			os << ' ';
			f(k);
		}
		os << ')';
	}
	return os << _weight->print_sym_info(sym);
}


std::vector<size_t> order_some_rule( TrsOrder& order, Trs::Rules const& rules ) {
	vector<pair<size_t,Smt::PostExp>> gts;
	auto& solver = order.solver();
	if( solver.is_sat() || solver.is_unsat() ) {
		solver.pop();
	}
	solver.push();
	for( auto const& [i,rule] : rules ) {
		if( order.log() & TermOrder::RULE ) {
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

std::function<PathOrder::Status(Trs::Rank const&)> PathOrder::Status::of( Exp const& x, bool mono ) {
	size_t n = 0;
	if( x.fun() == "straight" ) {
		x.get_end(n);
		return [](auto){ return Straight(); };
	}
	if( x.fun() == "map" ) {
		if( mono ) throw Error("#path-order","\"Monotone path-order with argument mapping is not supported.\"");
		int num;
		if( auto const& arg = x.gets_arg(n) ) {
			num = std::stoi(arg->unapplied().value_or_throw(Error("#malformed-number",*arg)));
		} else {
			num = -1;
		}
		return [num]( Trs::Rank const& rank ){
			return Mapped( num == -1 ? rank.arity : rank.arity ? num : 0 );
		};
	}
	throw Error("#malformed-status",x);
}
int TermOrder::log_of( Exp const& x ) {
	if( x == "rule" ) {
		return RULE;
	} else if( x == "pair" ) {
		return RULE | PAIR;
	} else if( x == "debug" ) {
		return RULE | PAIR | DEBUG;
	} else {
		throw Error("#unknown-log",x);
	}
}

std::unique_ptr<TrsOrder> TrsOrder::of(
	Exp const& x,
	Trs::Sig const& sig,
	Trs::Rules const& trs,
	bool mono,
	std::function<Smt::Solver()> const& default_smt,
	Smt::Sort const& default_sort,
	int default_log
) {
	auto const& f = x.fun();
	size_t n = 0;
	Opt<Exp> smt;
	auto mk_smt = [&]{ return smt ? Smt::Solver::of(*smt) : default_smt(); };
	int log = -1;
	auto set_log = [&]{ if( log == -1 ) log = default_log; };
	Exp::KeyValProc log_key = [&]( string_view const& key, Exp const& val ){
		if( key == "log" ) {
			if( log != -1 ) throw Error("#duplicate-log");
			log = log_of(val);
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
		x.process_keys( n, solver_key || log_key );
		set_log();
		return std::make_unique<DerivedTrsPosOrder<Poly>>
			( sig, trs, mono ? Poly::MONO_SUM : Poly::SUM, mk_smt(), log );
	} else if( f == "template" ) {
		Exp t = x.get_arg(n);
		x.process_keys( n, solver_key || log_key );
		set_log();
		return std::make_unique<DerivedTrsPosOrder<Poly>>(sig,trs,t,mk_smt(),log);
	} else if( f == "path-order" ) {
		auto w = x.gets_arg(n);
		Opt<std::function<PathOrder::Status(Trs::Rank const&)>> status;
		Exp::KeyValProc status_key = [&]( auto const& key, Exp const& val ){
			if( key == "status" ) {
				status = {PathOrder::Status::of(val,mono)};
				return true;
			}
			return false;
		};
		x.process_keys( n, status_key || log_key || (w ? [](auto,auto){ return false; } : solver_key) );
		x.get_end(n);
		set_log();
		return std::make_unique<PathOrder>(
			sig, trs,
			w ? of(*w,sig,trs,mono,default_smt,default_sort,log) : std::make_unique<TrivOrder>(mk_smt()),
			status ? *status : []( Trs::Rank const& rank ){ return PathOrder::Status::Straight(); },
			log
		);
	} else {
		throw Error("#unknown-order",x);
	}
}

Exp const SUM_SPEC = Exp{"sum"};
Exp const LPO3_SPEC = Exp{"path-order", ":status",Exp{"map","3"}};

void TermOrder::test() {
	cout << "=== TermOrder ===" << endl;
	auto z3 = Smt::Z3(Smt::LIA);
	auto x = z3.declare_const("x",Smt::INT);
	auto y = z3.declare_const("y",Smt::INT);
	auto z = z3.declare_const("z",Smt::INT);
	cout << lex_compare(Smt::compare,vector{x,y},{x,z}).ge << endl;
	cout << lex_compare(Smt::compare,vector{x},{x,z}).ge << endl;

	Trs::Sig sig = {{"+",{2}}};
	Trs::Rules trs;
	trs.emplace(0,Trs::Rule({"+","x","y"},{"x"}));

	auto lpo = PathOrder(sig,trs,std::make_unique<TrivOrder>
		(Smt::Z3(Smt::LIA)),[&](Trs::Rank const&){ return PathOrder::Status::Mapped(2); },TermOrder::RULE);
	cout << lpo.order_rule(0).gt << endl;

}