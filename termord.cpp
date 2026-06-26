#include "termord.hpp"
#include "template.hpp"
#include "poly.hpp"
#include "reach.hpp"

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
	_mono(_weight->solver().declare_const("MONO",Smt::BOOL)),
	_status(std::move(status))
{
	if( log & DEBUG ) cerr << "; monotonicity flag: " << _mono << endl;
}
void PathOrder::extend_sig( std::string const& f, Trs::Rank const& rank ) {
	auto& sol = solver();
	auto const& sort = sol.logic().base_sort();
	if( _log & DEBUG ) cerr << "; fun " << f << ' ' << rank << endl;
	_weight->extend_sig(f,rank);
	auto [info,suc] = _info.emplace(f,_SymInfo{sol.declare_const(string("p")+f,sort),rank.arity});
	assert(suc);
	sol.ass( Smt::ge(info.prec,0) );
	if( _log & DEBUG ) cerr << "; prec " << f << ": " << info.prec << endl;
	std::vector<Smt::PostExp> mapped_tbl;
	std::vector<Smt::PostExp> used_tbl;
	auto set_mappedi = [&]( Smt::PostExp const& mappedi, size_t i ) {
		if( _log & DEBUG ) cerr << "; monotonicity => mapped[" << f << ',' << i << "]" << endl;
		sol.ass( _mono.imp(mappedi) );
		if( _log & DEBUG ) cerr << "; mapped[" << f << ',' << i << "] => weight weak simple" << endl;
		sol.ass( mappedi.imp(_weight->arg_infl(f,i)) );
		if( _log & DEBUG ) cerr << "; used[" << f << ',' << i << "] := weight uses or mapped[" << i << "]" << endl;
		used_tbl.emplace_back(
			sol.define_fun( "u"+f+"_"+to_string(i), {}, Smt::BOOL, _weight->arg_used(f,i) || mappedi )
		);
	};
	if( auto post_arity = _status.fun(rank).post_arity() ) {
		info.post_arity = *post_arity;
		std::vector<std::vector<Smt::PostExp>> map_tbl;
		for( size_t i = 0; i < rank.arity; i++ ) {
			auto& mapi = map_tbl.emplace_back();
			for( size_t k = 0; k < info.post_arity; k++ ) {// k-th place after mapping
				auto const& ik = mapi.emplace_back(
					sol.declare_const(string("m")+f+"_"+to_string(i)+"_"+to_string(k),Smt::BOOL)
				);
				for( size_t j = 0; j < i; j++ ) {// k-th place cannot be shared
					sol.ass( !map_tbl[i][k] || !map_tbl[j][k] );
				}
			}
			if( _log & DEBUG ) cerr << ";  map[" << f << ',' << i << "] = " << print_list(mapi) << std::endl;
			// mapped[i] means i-th argument survives mapping
			auto const& mappedi = mapped_tbl.emplace_back(
				sol.define_fun("s"+f+"_"+to_string(i),{},Smt::BOOL,Smt::disj(mapi)) );
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
	info.used = [used_tbl=std::move(used_tbl)]( size_t i ){ return used_tbl[i]; };
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
	auto const& [rf,rargs] = *r;
	if( auto const& linfo = _info.find(lf) ) {// f(s...) >=? t
		Smt::PostExp some_arg_ge = false;
		for( size_t i = 0; i < largs.size(); i++ ) {
			auto [i_ge,i_gt] = compare(largs[i],r);
			// s > t if s_i survives and s_i >= t
			some_arg_ge = some_arg_ge || linfo->mapped(i) && i_ge;
		}
		Smt::PostExp gt_all_arg = true;
		if( auto const& rinfo = _info.find(rf) ) {// f(s...) >=? g(t...)
			for( size_t j = 0; j < rargs.size(); j++ ) {
				auto [ge_j,gt_j] = compare(l,rargs[j]);
				// if t_j survives, then s > t_j is prerequisite
				gt_all_arg = gt_all_arg && rinfo->mapped(j).imp(gt_j);
			}
			if( gt_all_arg == false ) return {false,false};
			auto const& [pge,pgt] = order(linfo->prec,rinfo->prec);
			auto const& [args_ge,args_gt] = mapped_lex_compare(
				[&]( auto const& x, auto const& y ){ return compare(x,y); },
				linfo->post_arity, rinfo->post_arity, linfo->map, rinfo->map, largs, rargs
			);
			if( _log & DEBUG ) {
				cerr << "; path_order: arguments [" << print_list(largs) << "] <=> [" << print_list(rargs) << "] = {" << args_ge << ", " << args_gt << '}' << endl;
			}
			gt_all_arg = solver().let(Smt::BOOL,gt_all_arg);
			auto const& gt = solver().let( Smt::BOOL,
				wgt || (wge && (some_arg_ge || ( gt_all_arg && ( pgt || ( pge && args_gt ) ) ) ) )
			);
			auto const& ge = solver().let( Smt::BOOL,
				gt || (wge && gt_all_arg && pge && args_ge )
			);
			return {ge,gt};
		} else {// f(s...) >=? y
			some_arg_ge = solver().let(Smt::BOOL,some_arg_ge);
			return {some_arg_ge,some_arg_ge};
		}
	} else {// x >=? t
		if( auto const& rinfo = _info.find(rf) ) {// x >=? g(t...)
			auto least = Smt::eq(0,rinfo->prec);// g must be least and no t_j survives
			for( size_t j = 0; j < rargs.size(); j++ ) {
				least = least && !rinfo->mapped(j);
			}
			return {least,false};
		} else {// x >=? y
			return { lf == rf, false };
		}
	}
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
	} else if( x == "use" ) {
		return RULE | PAIR | USE;
	} else if( x == "debug" ) {
		return RULE | PAIR | USE | DEBUG;
	} else {
		throw Error("#unknown-log",x);
	}
}

Smt::Compare TrsOrder::_Wrapper::rule_compare( size_t i, Trs::Term const& l, Trs::Term const& r ) {
	if( auto const& opt = _rule_order_table.find(i) ) return *opt;
	Smt::Solver& sol = _ref->solver();
	if( log() & RULE ) cerr << "; " << _ref->print_name() << ": rule-n " << i << ' ' << l << " <=> " << r << endl;
	auto [ge,gt] = _ref->compare(l,r);
	Smt::Compare ret = {sol.let(Smt::BOOL,ge),sol.let(Smt::BOOL,gt)};
	_rule_order_table.emplace(i,ret);
	return ret;
}

Smt::PostExp UsableRuleOrder::_Wrapper::term_used( Trs::Term const& r ) & {
	auto const& [g,rs] = *r;
	auto ret = Smt::conj(0,rs.size(),[&]( size_t p ){ return arg_used(g,p).imp(term_used(rs[p])); } );
	if( auto const& ginfo = _trs.sig.find(g) ) {
		return ret && Smt::conj( ginfo->defined_by, [&]( size_t i )->Smt::PostExp{
			if( auto const& rule = _trs.rules.find(i) )
				if( may_reach(_trs,r,rule->first,8,false) ) {//TODO
					if( log() & USE ) cerr << "; " << r << " uses " << i << endl;
					return rule_used(i);
				}
			return true;
		} );
	} else {
		return ret;
	}
}
Smt::PostExp UsableRuleOrder::_Wrapper::rule_used( size_t i ) & {
	if( auto const& ret = _usable_table.find(i) ) {
		return *ret;
	}
	auto rule = _trs.rules.find(i);
	if( !rule ) {// already removed
		return _usable_table.emplace(i,true).first;
	}
	auto const& [l,r,w] = *rule;
	auto& sol = _ref->solver();
	if( log() & DEBUG ) cerr << "; rule used i => term used " << r << endl;
	auto const& ret = _usable_table.emplace(i,sol.declare_const("used_"+to_string(i),Smt::BOOL)).first;
	sol.ass(ret.imp(term_used(r)));
	return ret;
}

std::unique_ptr<TermOrder> TermOrder::make(
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
		if( w ) weight = MemoizedTermOrder::make(make(*w,default_smt,default_sort,log));
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

std::unique_ptr<MemoizedTermOrder> MemoizedTermOrder::make( std::unique_ptr<TermOrder>&& ref ) {
	if( dynamic_cast<MemoizedTermOrder*>(ref.get()) ) {// do not wrap if origin is already memoized
		return std::unique_ptr<MemoizedTermOrder>(
			static_cast<MemoizedTermOrder*>(ref.release())
		);
	}
	return std::make_unique<_Wrapper>(std::move(ref));
}
std::unique_ptr<TrsOrder> TrsOrder::make( std::unique_ptr<TermOrder>&& ref ) {
	if( dynamic_cast<TrsOrder*>(ref.get()) ) {
		return std::unique_ptr<TrsOrder>(
			static_cast<TrsOrder*>(ref.release())
		);
	}
	return std::make_unique<_Wrapper>(std::move(ref));
}
std::unique_ptr<UsableRuleOrder> UsableRuleOrder::make( Trs const& trs, std::unique_ptr<TrsOrder>&& ref ) {
	if( dynamic_cast<UsableRuleOrder*>(ref.get()) ) {
		return std::unique_ptr<UsableRuleOrder>(
			static_cast<UsableRuleOrder*>(ref.release())
		);
	}
	return std::make_unique<_Wrapper>(trs,std::move(ref));
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

	auto lpo = TrsOrder::make(
		std::make_unique<PathOrder>(
			std::make_unique<TrivOrder>(Smt::Z3(Smt::LIA)),
			PathOrder::StatusFun( [&](Trs::Rank const&){ return PathOrder::Status::Mapped(2); } ),
			TermOrder::RULE
		)
	);
	lpo->extend_sig(sig);
	cout << lpo->rule_compare(0,{"+","x","y"},{"x"}).gt << endl;

}