#include "termord.hpp"
#include "template.hpp"
#include "poly.hpp"

using namespace std;

PathOrder::PathOrder(
	std::unique_ptr<TermOrder>&& weight,
	StatusFun&& status,
	int log
) : _weight(
		[&]{// ugly but C++ 
			if( log & DEBUG ) cerr << "; initializing path order" << endl;
			return std::move(weight);
		}()
	),
	_log(log),
	_mono(_weight->solver().declare_fresh(Smt::BOOL)),
	_status(std::move(status))
{
	if( log & DEBUG ) cerr << "; monotonicity flag: " << _mono << endl;
}
void PathOrder::extend_sig( std::string const& f, Trs::Rank const& rank ) {
	auto& sol = solver();
	auto const& sort = sol.logic().base_sort();
	if( _log & DEBUG ) cerr << "; fun " << f << ' ' << rank << endl;
	_weight->extend_sig(f,rank);
	auto [info,suc] = _info.emplace(f,_SymInfo{sol.declare_fresh(sort),rank.arity});
	assert(suc);
	sol.ass( Smt::ge(info.prec,0) );
	if( _log & DEBUG ) cerr << ";  prec: " << info.prec << endl;
	std::vector<Smt::PostExp> mapped_tbl;
	auto set_mappedi = [&]( Smt::PostExp const& mappedi, size_t i ) {
		// monotonicity requires mapped[i]
		sol.ass( _mono.imp(mappedi) );
		// mapped[i] requires weak simplicity of weight
		sol.ass( mappedi.imp(_weight->simple(f,i)) );
	};
	if( auto post_arity = _status.fun(rank).post_arity() ) {
		info.post_arity = *post_arity;
		std::vector<std::vector<Smt::PostExp>> map_tbl;
		for( size_t i = 0; i < rank.arity; i++ ) {
			auto& mapi = map_tbl.emplace_back();
			for( size_t k = 0; k < info.post_arity; k++ ) {// k-th place after mapping
				auto const& ik = mapi.emplace_back(sol.declare_fresh(Smt::BOOL));
				for( size_t j = 0; j < i; j++ ) {// k-th place cannot be shared
					sol.ass( !map_tbl[i][k] || !map_tbl[j][k] );
				}
			}
			if( _log & DEBUG ) cerr << ";  map[" << i << "] = " << print_list(mapi) << std::endl;
			// mapped[i] means i-th argument survives mapping
			auto const& mappedi = mapped_tbl.emplace_back( sol.let(Smt::BOOL,Smt::disj(mapi)) );
			set_mappedi(mappedi,i);
		}
		info.map = [map_tbl=std::move(map_tbl)]( size_t i, size_t j ){ return map_tbl[i][j]; };
	} else {// straight status
		for( size_t i = 0; i < rank.arity; i++ ) {
			auto const& mappedi = mapped_tbl.emplace_back(sol.declare_fresh(Smt::BOOL));
			set_mappedi(mappedi,i);
		}
		info.map = [&]( size_t i, size_t j ){
			return i == j ? info.mapped(i) : Smt::PostExp(false);
		};
	}
	info.mapped = [mapped_tbl=std::move(mapped_tbl)]( size_t i ){ return mapped_tbl[i]; };
}

Smt::Compare PathOrder::compare_inner( Exp const& l, Exp const& r ) {
	auto const& [wge,wgt] = _weight->compare(l,r);
	if( wgt == true ) {
		return {true,true};
	}
	if( wge == false ) {
		return {false,false};
	}
	auto const& [lf,largs] = *l;
	auto const& linfo = _info.find(lf);
	auto some_arg_ge = linfo ? Smt::disj( 0, largs.size(), [&]( size_t i ){
		return linfo->mapped(i) && compare(largs[i],r).ge;// l_i survives and l_i >= r
	} ) : false;
	if( some_arg_ge == true ) {
		return {true,true};
	}
	auto const& [rf,rargs] = *r;
	auto const& rinfo = _info.find(rf);
	auto gt_all_arg = rinfo ? Smt::conj( 0, rargs.size(), [&]( size_t j ){
		return rinfo->mapped(j).imp( compare(l,rargs[j]).gt );// if r_j survives, then l > r_j
	}) : true;
	if( !linfo ) {// lhs is a variable
		return { solver().let( Smt::BOOL, gt_all_arg && lf == rf ), false };
	}
	some_arg_ge = solver().let(Smt::BOOL,some_arg_ge);
	if( gt_all_arg == false ) return {false,false};
	if( !rinfo ) { // rhs is a variable
		return {some_arg_ge,some_arg_ge};
	}
	gt_all_arg = solver().let(Smt::BOOL,gt_all_arg);
	auto const& [args_ge,args_gt] = mapped_lex_compare(
		[&]( auto const& x, auto const& y ){ return compare(x,y); },
		linfo->post_arity, rinfo->post_arity, linfo->map, rinfo->map, largs, rargs
	);
	if( false && _log & DEBUG ) {
		cerr << "; [" << print_list(largs) << "] <=> [" << print_list(rargs) << "] = {" << args_ge << ", " << args_gt << '}' << endl;
	}
	auto const& [pge,pgt] = order(linfo->prec,rinfo->prec);
	auto const& gt = solver().let(
		Smt::BOOL, wgt || (wge && (some_arg_ge || ( gt_all_arg && ( pgt || ( pge && args_gt ) ) ) ) )
	);
	auto const& ge = solver().let(
		Smt::BOOL, gt || (wge && gt_all_arg && pge && args_ge )
	);
	return {ge,gt};
}

std::ostream& PathOrder::print_sym_info( std::ostream& os, std::string const& sym ) {
	auto info = _info.find(sym);
	assert(info);
	os << " :prec " << solver().get_value(info->prec);
	if( info->post_arity > 0 ) {
		os << " :map (";
		auto f = [&]( size_t k ){
			size_t i = 0;
			for(;;){
				if( i == info->arity ) {
					os << '-';
					break;
				}
				if( solver().get_value(info->map(i,k)) == Smt::TRUE ) {
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

PathOrder::StatusFun PathOrder::StatusFun::of( Exp const& x ) {
	size_t n = 0;
	if( x.fun() == "straight" ) {
		x.get_end(n);
		return StatusFun([](auto){ return Status::Straight(); });
	}
	if( x.fun() == "map" ) {
		int num;
		if( auto const& arg = x.gets_arg(n) ) {
			num = std::stoi(arg->unapplied().value_or_throw(Error("#malformed-number",*arg)));
		} else {
			num = -1;
		}
		return StatusFun([num]( Trs::Rank const& rank ){
			return Status::Mapped( num == -1 ? rank.arity : rank.arity ? num : 0 );
		});
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

std::unique_ptr<TermOrder> TermOrder::of(
	Exp const& x,
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
	} else if( f == "mono-sum" ) {
		x.process_keys( n, solver_key || log_key );
		set_log();
		return std::make_unique<DerivedTermOrder<MPoly>>(Template::MONO_SUM,mk_smt(),log);
	} else if( f == "sum" ) {
		x.process_keys( n, solver_key || log_key );
		set_log();
		return std::make_unique<DerivedTermOrder<MPoly>>(Template::SUM,mk_smt(),log);
	} else if( f == "poly" ) {
		x.process_keys( n, solver_key || log_key );
		set_log();
		return std::make_unique<DerivedTermOrder<MPoly>>(Template::MONO_POLY2,mk_smt(),log);
	} else if( f == "max" ) {
		x.process_keys( n, solver_key || log_key );
		set_log();
		return std::make_unique<DerivedTermOrder<MPoly>>(Template::MAX,mk_smt(),log);
	} else if( f == "template" ) {
		Exp t = x.get_arg(n);
		x.process_keys( n, solver_key || log_key );
		set_log();
		return std::make_unique<DerivedTermOrder<MPoly>>(t,mk_smt(),log);
	} else if( f == "path-order" ) {
		Opt<Exp> w;
		Exp::KeyValProc weight_key = [&]( auto const& key, Exp const& val ){
			if( key == "weight" ) {
				w = {val};
				return true;
			}
			return false;
		};
		Opt<PathOrder::StatusFun> status;
		Exp::KeyValProc status_key = [&]( auto const& key, Exp const& val ){
			if( key == "status" ) {
				status = {PathOrder::StatusFun::of(val)};
				return true;
			}
			return false;
		};
		x.process_keys( n, weight_key || status_key || log_key || (w ? [](auto,auto){ return false; } : solver_key) );
		x.get_end(n);
		set_log();
		std::unique_ptr<TermOrder> weight;
		if( w ) weight = std::make_unique<MemoizeTermOrder>(of(*w,default_smt,default_sort,log));
		else weight = std::make_unique<TrivOrder>(mk_smt());
		return std::make_unique<PathOrder>(
			std::move(weight),
			status ? *status : PathOrder::StatusFun( []( Trs::Rank const& rank ){ return PathOrder::Status::Straight(); } ),
			log
		);
	} else {
		throw Error("#unknown-order",x);
	}
}

std::unique_ptr<TrsOrder> TrsOrder::of(
	std::unique_ptr<TermOrder>&& p
) {
	if( dynamic_cast<TrsOrder*>(p.get()) ) {
		return std::unique_ptr<TrsOrder>(
			static_cast<TrsOrder*>(p.release())
		);
	}
	return std::make_unique<_Wrapper>(std::move(p));
}

Exp const SUM_SPEC = Exp{"sum"};
Exp const LPO_SPEC = Exp{"path-order", ":status", "map"};
Exp const LPO3_SPEC = Exp{"path-order", ":status",Exp{"map","3"}};

void TermOrder::test() {
	cout << "=== TermOrder ===" << endl;
	auto z3 = Smt::Z3(Smt::LIA);
	auto x = z3.declare_const("x",Smt::INT);
	auto y = z3.declare_const("y",Smt::INT);
	auto z = z3.declare_const("z",Smt::INT);
	cout << lex_compare(order,vector{x,y},{x,z}).ge << endl;
	cout << lex_compare(order,vector{x},{x,z}).ge << endl;

	Trs::Sig sig = {{"+",{2}}};

	auto lpo = TrsOrder::of(
		std::make_unique<PathOrder>(
			std::make_unique<TrivOrder>(Smt::Z3(Smt::LIA)),
			PathOrder::StatusFun( [&](Trs::Rank const&){ return PathOrder::Status::Mapped(2); } ),
			TermOrder::RULE
		)
	);
	lpo->extend_sig(sig);
	cout << lpo->order_rule({"+","x","y"},{"x"},0).gt << endl;

}