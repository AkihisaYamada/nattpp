#include<sstream>
#include "termord.hpp"
#include "template.hpp"
#include "poly.hpp"
#include "reach.hpp"
#include "tpalgebra.hpp"

using namespace std;

std::string escape( std::string const& str ) {
	auto ss = std::ostringstream();
	for( auto const& c : str ) {
		switch(c) {
		case '#':  ss << "<sh>"; break;
		case '|':  ss << "<hl>"; break;
		case '\'': ss << "<sq>"; break;
		case '\"': ss << "<dq>"; break;
		case '`':  ss << "<bq>"; break;
		case ',':  ss << "<cm>"; break;
		case ':':  ss << "<cl>"; break;
		case ';':  ss << "<sc>"; break;
		case '<':  ss << "<lb>"; break;
		case '>':  ss << "<rb>"; break;
		case '\\': ss << "<bs>"; break;
		default: ss << c; break;
		}
	}
	return ss.str();
}

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
	if( _info.find(f) ) return;
	auto& sol = solver();
	auto const& sort = sol.logic().base_sort();
	if( _log & DEBUG ) cerr << "; fun " << f << ' ' << rank << endl;
	_weight->extend_sig(f,rank);
	auto prec = sol.declare_const("p"+escape(f),sol.logic().base_sort());//sol.declare_fresh(sort);
	sol.ass( Smt::ge(prec,0) );
	std::string fesc = escape(f);
	if( _log & DEBUG ) cerr << "; prec " << f << ": " << prec << endl;
	uint16_t arity = rank.arity;
	Status status = _status.fun(rank);
	if( auto post_arity_opt = status.post_arity() ) {
		uint16_t post_arity = *post_arity_opt;
		if( post_arity == 0 ) {// empty status
			_info.emplace(f,_SymInfo{
				.arity = arity,
				.status = status,
				.prec = prec,
				.collapse = false,
				.empty = true,
				.map = []( uint16_t i, uint16_t j ){ return false; },
				.mapped = []( uint16_t i ){ return false; },
				.occupied = []( uint16_t i ){ return false; },
				.used = [&]( uint16_t i ){ return _weight->arg_used(f,i); },
			});
		} else {
			std::vector<Smt::PostExp> used_tbl;
			auto collapse = sol.declare_const(fesc+"_c",Smt::BOOL);
			if( _log & DEBUG ) cerr << "; collapse[" << f << "] => trivial weight" << endl;
			sol.ass(collapse.imp(_weight->fun_triv(f)));
			std::vector<Smt::PostExp> mapped_tbl;
			std::vector<std::vector<Smt::PostExp>> map_tbl;
			for( uint16_t i = 0; i < arity; i++ ) {
				auto& mapi = map_tbl.emplace_back();
				for( uint16_t k = 0; k < post_arity; k++ ) {// k-th place after mapping
					auto const& ik = mapi.emplace_back(
						sol.declare_const(fesc+"_m"+to_string(i)+"_"+to_string(k),Smt::BOOL)
					);
					for( uint16_t j = 0; j < i; j++ ) {// k-th place cannot be shared
						sol.ass( !map_tbl[i][k] || !map_tbl[j][k] );
					}
				}
				if( _log & DEBUG ) cerr << ";  map[" << f << ',' << i << "] = " << print_list(mapi) << std::endl;
				// mapped[i] means i-th argument survives mapping
				auto const& mappedi = mapped_tbl.emplace_back(
					sol.define_fun( fesc+"_s"+to_string(i), {}, Smt::BOOL, Smt::PostExp::disj(mapi))
				);
				if( _log & DEBUG ) cerr << "; monotonicity => mapped[" << f << ',' << i << "]" << endl;
				sol.ass( _mono.imp(mappedi) );
				if( _log & DEBUG ) cerr << "; mapped[" << f << ',' << i << "] => weight weak simple" << endl;
				sol.ass( mappedi.imp(_weight->arg_infl(f,i)) );
				if( _log & DEBUG ) cerr << "; used[" << f << ',' << i << "] := weight used or mapped[" << i << "]" << endl;
				used_tbl.emplace_back(
					sol.define_fun( fesc+"_u"+to_string(i), {},
						Smt::BOOL, _weight->arg_used(f,i) || mappedi
					)
				);
				if( _log & DEBUG ) cerr << "; collapse[" << f << "] => weight used => mapped" << endl;
				sol.ass( collapse.imp( _weight->arg_used(f,i).imp(mappedi) ) ); 
			}
			if( _log & DEBUG ) cerr << "; require argument mapping to be contiguous" << endl;
			std::vector<Smt::PostExp> occupied;
			auto add_occupied = [&]( uint16_t k )->Smt::PostExp const&{
				return occupied.emplace_back(
					sol.define_fun(fesc+"_o"+to_string(k),{},
						Smt::BOOL,Smt::PostExp::disj(0u,rank.arity,[&]( unsigned int const& i )->Smt::PostExp{ return map_tbl[i][k]; })
					)
				);
			};
			add_occupied(0);
			for( uint16_t k = 1; k < post_arity; k++ ) {
				auto& next = add_occupied(k);
				sol.ass(next.imp(occupied[k-1]));
			}
			if( _log & DEBUG ) {
				cerr << "; post-position occupancy: (" << print_list(occupied) << ")" << endl;
				cerr << "; collapse => occupied[0] && !occupied[1]" << endl;
			}
			sol.ass(collapse.imp( occupied[0] && (post_arity > 1 ? !occupied[1] : Smt::PostExp(true) )));
			_info.emplace(f,_SymInfo{
				.arity = arity,
				.status = status,
				.prec = prec,
				.collapse = collapse,
				.empty = !occupied[0],
				.map = [map_tbl=std::move(map_tbl),arity,post_arity]( uint16_t i, uint16_t j )->Smt::PostExp{
					if( i < arity && j < post_arity ) return map_tbl[i][j];
					return false;
				},
				.mapped = [mapped_tbl=std::move(mapped_tbl),arity]( uint16_t i )->Smt::PostExp{
					if( i < arity ) return mapped_tbl[i];
					return false;
				},
				.occupied = [occupied=std::move(occupied),post_arity]( uint16_t i )->Smt::PostExp{
					if( i < post_arity ) return occupied[i];
					return false;
				},
				.used = [used_tbl=std::move(used_tbl)]( uint16_t i ){ return used_tbl[i]; }
			});
		}
	} else {// straight status
		if( _log & DEBUG ) cerr << "; straight status requires inflationarity on all arguments" << endl;
		sol.ass( Smt::PostExp::conj( 0, arity, [&]( uint16_t i ){ return _weight->arg_infl(f,i); } ) );
		_info.emplace(f,_SymInfo{
			.arity = arity,
			.status = status,
			.prec = prec,
			.collapse = false,
			.empty = arity == 0,
			.map = []( uint16_t i, uint16_t j ){ return i == j; },
			.mapped = []( uint16_t i ){ return true; },
			.occupied = [arity]( uint16_t i ){ return i < arity; },
			.used = []( uint16_t i ){ return true; }
		});
	}
}

Smt::Compare PathOrder::compare_inner( Exp const& l, Exp const& r ) {
	auto const& [wge,wgt] = _weight->compare(l,r);
	if( wge == false ) {
		return {false,false};
	}
	auto const& [lf,largs] = *l;
	auto const& linfo = _info.find(lf);
	if( !linfo ) {// x >=? t
		auto const& [rf,rargs] = *r;
		if( auto const& rinfo = _info.find(rf) ) {// x >=? g(t...)
			Smt::PostExp ge_all_arg = true, gt_all_arg = true;
			for( uint16_t j = 0; j < rinfo->arity; j++ ) {
				auto const& [gej,gtj] = compare(l,rargs[j]);// just to invoke memoization
				ge_all_arg = ge_all_arg && rinfo->mapped(j).imp(gej);
				gt_all_arg = gt_all_arg && rinfo->mapped(j).imp(gtj);
			}
			return {
				wge && (
					rinfo->collapse && ge_all_arg ||// g is collapsed
					Smt::eq(0,rinfo->prec) && rinfo->empty// g is least and no t_j survives
				),
				wgt || rinfo->collapse && gt_all_arg
			};
		} else {// x >=? y
			return {l == r, false};
		}
	}
	// f(s...) >=? t
	auto const& [rf,rargs] = *r;
	auto& sol = solver();
	Smt::PostExp some_arg_ge = false;
	Smt::PostExp some_arg_gt = false;
	for( uint16_t i = 0; i < largs.size(); i++ ) {
		auto const& i_mapped = linfo->mapped(i);
//			if( i_mapped == false ) break;
		auto const& [i_ge,i_gt] = compare(largs[i],r);
		some_arg_ge = some_arg_ge || i_mapped && i_ge;// s_i survives and s_i ≥ t
		some_arg_gt = some_arg_gt || i_mapped && i_gt;
	}
	auto const& rinfo = _info.find(rf);
	if( !rinfo ) {// f(s..) >=? y
		some_arg_ge = sol.let(Smt::BOOL,some_arg_ge);
		auto const& gt = sol.let( Smt::BOOL,
			wgt || linfo->collapse && some_arg_gt || !linfo->collapse && some_arg_ge
		);
		return { gt || some_arg_ge, gt };
	}
	// f(s...) >=? g(t...)
	Smt::PostExp gt_all_arg = true;
	Smt::PostExp ge_all_arg = true;
	for( uint16_t j = 0; j < rargs.size(); j++ ) {
		auto const& j_mapped = rinfo->mapped(j);
//				if( j_mapped == false ) break;
		auto const& [ge_j,gt_j] = compare(l,rargs[j]);
		ge_all_arg = ge_all_arg && j_mapped.imp(ge_j);
		gt_all_arg = gt_all_arg && j_mapped.imp(gt_j); // if t_j survives, then s > t_j is prerequisite
	}
	if( gt_all_arg == false ) return { false, false };
	auto const& [pge,pgt] = order(linfo->prec,rinfo->prec);
	auto const& [args_ge,args_gt] = mapped_lex_compare(
		sol, [&]( auto const& x, auto const& y ){ return compare(x,y); },
		linfo->occupied, rinfo->occupied, linfo->map, rinfo->map, largs, rargs
	);
	if( _log & DEBUG ) {
		cerr << "; path_order: arguments [" << print_list(largs) << "] >=? [" << print_list(rargs) << "] = {" << args_ge << ", " << args_gt << '}' << endl;
	}
	some_arg_ge = sol.let(Smt::BOOL,some_arg_ge);
	gt_all_arg = sol.let(Smt::BOOL,gt_all_arg);
	auto const& gt = sol.let( Smt::BOOL,
		wgt ||
		wge && (
			some_arg_gt ||
			(!linfo->collapse && (
				some_arg_ge ||
					(gt_all_arg &&
					( rinfo->collapse || pgt || ( pge && args_gt ) ))
			))
		)
	);
	auto const& ge = sol.let( Smt::BOOL,
		gt ||
		(linfo->collapse && some_arg_ge) ||
		(rinfo->collapse && ge_all_arg) ||
		(!linfo->collapse && !rinfo->collapse && wge && gt_all_arg && pge && args_ge)
	);
	return {ge,gt};
}

std::ostream& PathOrder::print_sym_info( std::ostream& os, std::string const& sym ) {
	auto info = _info.find(sym);
	assert(info);
	auto& sol = solver();
	os << " :prec " << sol.get_value(info->prec);
	if( auto post_arity = info->status.post_arity(); post_arity && *post_arity > 0 ) {
		auto f = [&]( string_view const& prefix, uint16_t k ){
			uint16_t i = 0;
			for(;;){
				if( i == info->arity ) {
					return false;
				}
				if( solver().get_value(info->map(i,k)).as_bool() ) {
					os << prefix << i+1;
					return true;
				}
				i++;
			}
		};
		if( sol.get_value(info->collapse).as_bool() ) {
			os << " :collapse (";
		} else {
			os << " :map (";
		}
		f("",0);
		for( uint16_t k = 1; f(" ",k); k++ );
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
	} else if( x == "init" ) {
		return RULE | PAIR | USE | INIT;
	} else if( x == "debug" ) {
		return RULE | PAIR | USE | INIT | DEBUG;
	} else {
		throw Error("#unknown-log",x);
	}
}

Smt::Compare TrsOrder::_Wrapper::rule_compare( size_t i, Trs::Term const& l, Trs::Term const& r ) {
	if( auto const& opt = _rule_order_table.find(i) ) return *opt;
	Smt::Solver& sol = _ref->solver();
	if( log() & RULE ) cerr << "; " << _ref->print_name() << ": (rule-n " << i << ' ' << l << ' ' << r << ')' << endl;
	auto [ge,gt] = _ref->compare(l,r);
	Smt::Compare ret = {sol.let(Smt::BOOL,ge),sol.let(Smt::BOOL,gt)};
	_rule_order_table.emplace(i,ret);
	return ret;
}

Smt::PostExp UsableRuleOrder::_Wrapper::term_used( Trs::Term const& r ) & {
	auto const& [g,rs] = *r;
	auto ret = Smt::PostExp::conj(0,rs.size(),[&]( uint16_t p ){ return arg_used(g,p).imp(term_used(rs[p])); } );
	if( auto const& ginfo = _trs.sig.find(g) ) {
		return ret && Smt::PostExp::conj( ginfo->defined_by, [&]( uint16_t i )->Smt::PostExp{
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
	bool mono;
	Exp::KeyValProc mono_key = [&]( string_view const& key, Exp const& val ){
		if( key == "mono" ) {
			mono = val.as_bool();
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
		return std::make_unique<DerivedTermOrder<Poly>>(Poly::ALGEBRA,Template::MONO_SUM,mk_smt(),true,log);
	} else if( f == "sum" ) {
		x.process_keys( n, solver_key || log_key );
		set_log();
		return std::make_unique<DerivedTermOrder<Poly>>(Poly::ALGEBRA,Template::SUM,mk_smt(),false,log);
	} else if( f == "mono-bpoly" ) {
		x.process_keys( n, solver_key || log_key );
		set_log();
		return std::make_unique<DerivedTermOrder<Poly>>(Poly::ALGEBRA,Template::MONO_POLY2,mk_smt(),true,log);
	} else if( f == "mat2b" ) {
		x.process_keys( n, solver_key || log_key );
		set_log();
		return std::make_unique<DerivedTermOrder<TupleVal<Poly>>>(
			tuple_algebra<Poly::Range,Poly>({Poly::POS,Poly::POS}),Template::MAT2B,mk_smt(),false,log
		);
	} else if( f == "mat2n" ) {
		x.process_keys( n, solver_key || log_key );
		set_log();
		return std::make_unique<DerivedTermOrder<TupleVal<Poly>>>(
			tuple_algebra<Poly::Range,Poly>({Poly::POS,Poly::POS}),Template::MAT2N,mk_smt(),false,log
		);
	} else if( f == "max" ) {
		x.process_keys( n, solver_key || log_key );
		set_log();
		return std::make_unique<DerivedTermOrder<MPoly>>(MPoly::ALGEBRA,Template::MAX,mk_smt(),false,log);
	} else if( f == "imax" ) {
		x.process_keys( n, solver_key || log_key );
		set_log();
		return std::make_unique<DerivedTermOrder<MPoly>>(MPoly::ALGEBRA,Template::IMAX,mk_smt(),false,log);
	} else if( f == "template" ) {
		Exp t = x.get_arg(n);
		x.process_keys( n, solver_key || log_key || mono_key );
		set_log();
		return std::make_unique<DerivedTermOrder<MPoly>>(MPoly::ALGEBRA,t,mk_smt(),mono,log);
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
			RULE
		)
	);
	lpo->extend_sig(sig);
	cout << lpo->rule_compare(0,{"+","x","y"},{"x"}).gt << endl;

}