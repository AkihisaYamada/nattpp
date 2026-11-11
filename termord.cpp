#include<sstream>
#include "termord.hpp"
#include "template.hpp"
#include "poly.hpp"
#include "reach.hpp"
#include "tpalgebra.hpp"

using namespace std;

std::string escape( std::string const& str ) {
	static constexpr char const h[] = "0123456789ABCDEF";
	auto ss = std::ostringstream();
	for( unsigned char c : str ) {
		if( isalnum(c) ) {
			ss << c;
			continue;
		}
		switch(c) {
		case '+': case '-': case '*': case '/': case '@': case '$': case '^': case '&': case '_': case '=': case '<': case '>': case '~': case '.': case '!': case '?':
			ss << c;
			continue;
		}
		ss << '%' << h[c>>4] << h[c&15];
	}
	return ss.str();
}

PathOrder::PathOrder(
	std::unique_ptr<TermOrder>&& weight,
	StatusFun&& status,
	int log
) : _weight(
		[&]{// ugly but C++ 
			if( log & INIT ) cerr << "; initializing path order" << endl;
			return std::move(weight);
		}()
	),
	_log(log),
	_mono(_weight->solver().declare_const("MONO",Smt::BOOL)),
	_status(std::move(status))
{
	if( log & INIT ) cerr << "; monotonicity flag: " << _mono << endl;
}
void PathOrder::extend_sig( std::string const& f, Trs::Rank const& rank, bool heuristic ) {
	if( _info.find(f) ) return;
	auto& sol = solver();
	auto const& sort = sol.logic().base_sort();
	if( _log & INIT ) cerr << "; fun " << f << ' ' << rank.print_content() << endl;
	_weight->extend_sig(f,rank,heuristic);
	std::string fesc = escape(f);
	auto prec = sol.declare_const("p_"+fesc,sol.logic().base_sort());//sol.declare_fresh(sort);
	sol.ass( prec.ge(0) );
	if( _log & INIT ) cerr << "; prec " << f << ": " << prec << endl;
	uint16_t arity = rank.arity;
	Status status = _status(f,rank);
	if( auto map_opt = status.mapped() ) {
		uint16_t post_arity = map_opt->post_arity;
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
			Smt::PostExp collapse =
				map_opt->collapse == Collapse::AUTO ? sol.declare_const("c_"+fesc,Smt::BOOL) :
				map_opt->collapse == Collapse::FORCE ? true :
				false;
			auto col_imp_triv = collapse.imp(_weight->fun_triv(f));
			if( _log & INIT ) cerr << "; collapse[" << f << "] => trivial weight: " << col_imp_triv << endl;
			sol.ass(col_imp_triv);
			std::vector<Smt::PostExp> mapped_tbl;
			std::vector<std::vector<Smt::PostExp>> map_tbl;
			for( uint16_t i = 0; i < arity; i++ ) {
				auto& mapi = map_tbl.emplace_back();
				for( uint16_t k = 0; k < post_arity; k++ ) {// k-th place after mapping
					auto const& ik = mapi.emplace_back(
						sol.declare_const("m_"+fesc+"_"+to_string(i)+"_"+to_string(k),Smt::BOOL)
					);
					for( uint16_t j = 0; j < i; j++ ) {// k-th place cannot be shared
						sol.ass( !map_tbl[i][k] || !map_tbl[j][k] );
					}
				}
				if( _log & INIT ) cerr << ";  map[" << f << ',' << i << "] = {" << print_list(mapi) << '}' << std::endl;
				// mapped[i] means i-th argument survives mapping
				auto const& mappedi = mapped_tbl.emplace_back(
					sol.define_fun( "s_"+fesc+"_"+to_string(i), {}, Smt::BOOL, Smt::PostExp::disj(mapi))
				);
				if( _log & INIT ) cerr << "; monotonicity => mapped[" << f << ',' << i << "]" << endl;
				sol.ass( _mono.imp(mappedi) );
				auto map_imp_infl = mappedi.imp(_weight->arg_infl(f,i));
				if( _log & INIT ) cerr << "; mapped[" << f << ',' << i << "] => weight weak simple: " << map_imp_infl << endl;
				sol.ass( map_imp_infl );
				used_tbl.emplace_back(_weight->arg_used(f,i));
				auto col_imp_used_imp_mapped = collapse.imp( _weight->arg_used(f,i).imp(mappedi) );
				if( _log & INIT ) cerr << "; collapse[" << f << "] => weight used => mapped: " << col_imp_used_imp_mapped << endl;
				sol.ass( col_imp_used_imp_mapped ); 
			}
			if( _log & INIT ) cerr << "; require argument mapping to be contiguous" << endl;
			std::vector<Smt::PostExp> occupied;
			auto add_occupied = [&]( uint16_t k )->Smt::PostExp const&{
				return occupied.emplace_back(
					sol.define_fun("o_"+fesc+"_"+to_string(k),{},
						Smt::BOOL,Smt::PostExp::disj(0u,rank.arity,[&]( unsigned int const& i )->Smt::PostExp{ return map_tbl[i][k]; })
					)
				);
			};
			add_occupied(0);
			for( uint16_t k = 1; k < post_arity; k++ ) {
				auto& next = add_occupied(k);
				sol.ass(next.imp(occupied[k-1]));
			}
			auto collapse_imp_ex1 =
				collapse.imp( occupied[0] && (post_arity > 1 ? !occupied[1] : Smt::PostExp(true) ) );
			if( _log & INIT ) {
				cerr << "; post-position occupancy: {" << print_list(occupied) << "}" << endl;
				cerr << "; collapse => exactly one post argument is occupied: " << collapse_imp_ex1 << endl;
			}
			sol.ass(collapse_imp_ex1);
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
		if( _log & INIT ) cerr << "; straight status requires inflationarity on all arguments" << endl;
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

Smt::Compare PathOrder::_compare_sub( _TermComp const& comp_sub, _TermComp const& w_comp, _SymComp const& p_comp, Exp const& l, Exp const& r ) {
	auto [w_ge,w_str] = w_comp(l,r);
	if( w_ge == false ) {
		return {false,false};
	}
	auto const& [lf,largs] = *l;
	auto const& linfo = _info.find(lf);
	if( !linfo ) {// x >=? t
		auto const& [rf,rargs] = *r;
		auto const& rinfo = _info.find(rf);
		if( !rinfo ) {// x >=? y
			return { l == r, false };// variables must be identical, gl cannot be ensured
		}
		// x >=? g(t...)
		Smt::PostExp ge_all_arg = true, str_all_arg = true;
		for( uint16_t j = 0; j < rinfo->arity; j++ ) {
			auto const& [j_ge,j_str] = comp_sub(l,rargs[j]);
			ge_all_arg = ge_all_arg && rinfo->mapped(j).imp(j_ge);
			str_all_arg = str_all_arg && rinfo->mapped(j).imp(j_str);
		}
		auto ge = w_ge && (
			rinfo->collapse && ge_all_arg ||// g is collapsed
			Smt::PostExp(0).eq(rinfo->prec) && rinfo->empty// g is least and no t_j survives
		);
		if( ge == false ) return {false,false};
		return { ge, w_str || rinfo->collapse && str_all_arg };
	}
	// f(s...) >=? t
	auto const& [rf,rargs] = *r;
	auto& sol = solver();
	Smt::PostExp some_arg_ge = false;
	Smt::PostExp some_arg_str = false;
	for( uint16_t i = 0; i < largs.size(); i++ ) {
		auto const& i_mapped = linfo->mapped(i);
//			if( i_mapped == false ) break;
		auto const& [i_ge,i_str] = comp_sub(largs[i],r);
		some_arg_ge = some_arg_ge || i_mapped && i_ge;// s_i survives and s_i ≥ t
		some_arg_str = some_arg_str || i_mapped && i_str;
	}
	auto const& rinfo = _info.find(rf);
	if( !rinfo ) {// f(s..) >=? y
		w_str = sol.let(Smt::BOOL,w_str);
		return { w_str || some_arg_ge,
			w_str || linfo->collapse.imp(some_arg_str)// if f is collapsed, s = y is weak
		};
	}
	// f(s...) >=? g(t...)
	Smt::PostExp ge_all_arg = true;
	Smt::PostExp str_all_arg = true;
	for( uint16_t j = 0; j < rargs.size(); j++ ) {
		auto const& j_mapped = rinfo->mapped(j);
//				if( j_mapped == false ) break;
		auto const& [ge_j,str_j] = comp_sub(l,rargs[j]);
		ge_all_arg = ge_all_arg && j_mapped.imp(ge_j);
		str_all_arg = str_all_arg && j_mapped.imp(str_j); // if t_j survives, then s > t_j is prerequisite
	}
//	if( ge_all_arg == false ) return { false, false };
	auto const& [p_ge,p_str] = p_comp(*linfo,*rinfo);
	auto const& [args_ge,args_str] = mapped_lex_order(
		sol, [&]( auto const& x, auto const& y ){ return comp_sub(x,y); },
		linfo->occupied, rinfo->occupied, linfo->map, rinfo->map, largs, rargs
	);
	if( _log & DEBUG ) {
		cerr << "; path-order: arguments (" << lf;
		for( auto const& a : largs ) cerr << ' ' << a;
		cerr << ") >=? (" << rf;
		for( auto const& a : rargs ) cerr << ' ' << a;
		cerr << ") = {" << args_ge << ", " << args_str << '}' << endl;
	}
	some_arg_ge = sol.let(Smt::BOOL,some_arg_ge);
	str_all_arg = sol.let(Smt::BOOL,str_all_arg);
	auto const& ge = sol.let( Smt::BOOL,
		w_ge && (
			w_str ||
			some_arg_ge ||
			ge_all_arg && (
				rinfo->collapse ||// s ≥ t_j is enough if g is collapsed
				str_all_arg &&// s > t_j is required if g survives
				// compare precedence if f and g survive
				!linfo->collapse && !rinfo->collapse && p_ge && (p_str || args_ge)
			)
		)
	);
	if( ge == false ) return {false,false};
	auto const& str =
		w_str ||// w(s) > w(t)
		some_arg_ge && linfo->collapse.imp(some_arg_str) ||// s_i > t
		rinfo->collapse && str_all_arg ||
		!linfo->collapse && !rinfo->collapse && (p_str || args_str);
	return {ge,str};
}

std::ostream& PathOrder::print_sym_info( std::ostream& os, std::string const& sym ) {
	auto info = _info.find(sym);
	assert(info);
	auto& sol = solver();
	os << " :prec " << sol.get_value(info->prec);
	if( auto mapped = info->status.mapped(); mapped && mapped->post_arity > 0 ) {
		auto f = [&]( string_view const& prefix, uint16_t k ){
			uint16_t i = 0;
			for(;;){
				if( i == info->arity ) {
					return false;
				}
				if( solver().get_value(info->map(i,k)).as_bool() ) {
					os << prefix << i;
					return true;
				}
				i++;
			}
		};
		if( sol.get_value(info->collapse).as_bool() ) {
			os << " :collapse ";
			f("",0);
		} else {
			os << " :map (";
			f("",0);
			for( uint16_t k = 1; f(" ",k); k++ );
			os << ')';
		}
	}
	return os << _weight->print_sym_info(sym);
}

PathOrder::Collapse collapse_mode_of( Exp const& x ) {
	auto v = x.as_unapplied();
	if( v == "none" ) return PathOrder::Collapse::NONE;
	if( v == "auto" ) return PathOrder::Collapse::AUTO;
	if( v == "force" ) return PathOrder::Collapse::FORCE;
	throw Error("#path-order","\"unknown collapse mode\"",x);
}

PathOrder::Status _status_of( Exp const& x, std::string const& name, Trs::Rank const& rank ) {
	uint i = 0;
	if( x.fun() == "straight" ) {
		x.get_end(i);
		return PathOrder::Status::Straight();
	}
	if( x.fun() == "map" ) {
		int postarity = -1;
		Opt<PathOrder::Collapse> ocollapse;
		x.process_keys(i,
			Exp::OnKey("post-arity") >> [&]( auto const& val ){
				postarity = val.as_nat([](auto n){ return n < 32; });
			} || Exp::OnKey("collapse") >> [&]( auto const& val ){
				ocollapse = {collapse_mode_of(val)};
			}
		);
		return PathOrder::Status::Mapped(
			postarity == -1 ? rank.arity : rank.arity ? postarity : 0,
			ocollapse.value_or(PathOrder::Collapse::AUTO)
		);
	}
	if( x.fun() == "cond" ) {
		while( auto const& xi = x.gets_arg(i) ) {
			uint j = 0;
			if( xi->fun() == "name" ? xi->get_arg(j).as_unapplied() == name :
				xi->fun() == "arity" ? Exp(xi->get_arg(j)).as_nat([](auto n){ return n < 255; }) :
				xi->fun() == "otherwise" || (throw Error("#path-order:status","\"unknown cond\"",*xi), false)
			) {
				auto xj = xi->get_arg(j);
				xi->get_end(j);
				return _status_of(xj,name,rank);
			}
		}
		throw Error("#path-order:status","\"unmatching cond\"",name);
	}
	throw Error("#path-order","\"malformed status\"",x);
}

PathOrder::StatusFun PathOrder::status_fun_of( Exp const& x ) {
	uint i = 0;
	if( x.fun() == "straight" ) {
		x.get_end(i);
		return [](auto,auto){ return Status::Straight(); };
	}
	if( x.fun() == "map" ) {
		int postarity = -1;
		Opt<Collapse> ocollapse;
		x.process_keys(i,
			Exp::OnKey("post-arity") >> [&]( auto const& val ){
				postarity = val.as_nat([](auto n){ return n < 32; });
			} || Exp::OnKey("collapse") >> [&]( auto const& val ){
				ocollapse = {collapse_mode_of(val)};
			}
		);
		return [postarity,collapse=ocollapse.value_or(Collapse::AUTO)]( std::string const&, Trs::Rank const& rank ){
			return Status::Mapped( postarity == -1 ? rank.arity : rank.arity ? postarity : 0, collapse );
		};
	}
	return [x]( std::string const& name, Trs::Rank const& rank ){
		return _status_of(x,name,rank);
	};
}
int TermOrder::log_of( Exp const& x ) {
	if( x == "init" ) {
		return INIT;
	} else if( x == "rule" ) {
		return INIT | RULE;
	} else if( x == "pair" ) {
		return INIT | RULE | PAIR;
	} else if( x == "use" ) {
		return INIT | RULE | PAIR | USE;
	} else if( x == "debug" ) {
		return INIT | RULE | PAIR | USE | DEBUG;
	} else {
		throw Error("#unknown-log",x);
	}
}

Smt::Compare TrsOrder::_Wrapper::rule_order( ruleId i, Trs::Term const& l, Trs::Term const& r ) {
	if( auto const& opt = _rule_order_cache.find(i) ) return *opt;
	Smt::Solver& sol = _ref->solver();
	if( log() & RULE ) cerr << "; " << _ref->print_name() << ": (rule-n " << i << ' ' << l << ' ' << r << ')' << endl;
	auto [ge,gt] = _ref->order(l,r);
	Smt::Compare ret = {sol.let(Smt::BOOL,ge),sol.let(Smt::BOOL,gt)};
	_rule_order_cache.emplace(i,ret);
	return ret;
}
Smt::PostExp TrsOrder::_Wrapper::edge_co_order( ruleId i, ruleId j, Problem const& p ) {
	auto& cache = _edge_separate_cache.find(i) || [&]()->Map<ruleId,Smt::PostExp>&{
		return _edge_separate_cache.emplace(i,Map<ruleId,Smt::PostExp>{}).first;
	};
	if( auto const& o = cache.find(j) ) {
		return *o;
	}
	auto src = ASSERTED(p.rules.find(i))->second;
	auto [f,ss] = *src;
	auto tgt = get_renamed_lhs(j,p);
	auto [g,ts] = *tgt;
	assert( f == g && ss.size() == ts.size() );
	Smt::PostExp sep = false;
	for( size_t i = 0; i < ss.size(); i++ ) {
		 sep = sep || _ref->separate(ss[i],ts[i]);
	}
	if( log() & SEP ) {
		cerr << "; (>? " << src << ' ' << tgt << ") = " << sep << endl;
	}
	auto ret = _ref->solver().let(Smt::BOOL,sep);
	cache.emplace(j,ret);
	return ret;
}
Smt::PostExp TrsOrder::_Wrapper::order_by_edge( ruleId i, ruleId j, Problem const& p ) {
	auto& cache = _order_by_edge_cache.find(i) || [&]()->Map<ruleId,Smt::PostExp>&{
		return _order_by_edge_cache.emplace(i,Map<ruleId,Smt::PostExp>{}).first;
	};
	if( auto const& o = cache.find(j) ) {
		return *o;
	}
	auto [l,r,w] = *ASSERTED(p.rules.find(i));
	auto tgt = get_renamed_lhs(j,p);
	auto gt_if = _ref->gt_if(l,r,tgt);
	// Declare a boolean variable that implies orientation, but not necessarily vise versa.
	// This is necessary for when the exact value does not contribute to satisfiability.
	auto ret = gt_if.is_bool() ? gt_if : [&]{
		auto var = solver().declare_fresh(Smt::BOOL);
		if( log() & PAIR ) {
			cerr << "; (>_if " << l << ' ' << r << ' ' << tgt << ") = " << gt_if << endl;
		}
		solver().ass(var.imp(gt_if));
		return var;
	}();
	cache.emplace(j,ret);
	return ret;
}

Smt::PostExp UsableRuleOrder::_Wrapper::term_used( Trs::Term const& r ) & {
	auto const& [g,rs] = *r;
	auto ret = Smt::PostExp::conj(0,rs.size(),[&]( uint16_t p ){ return arg_used(g,p).imp(term_used(rs[p])); } );
	if( auto const& ginfo = _trs.sig.find(g); ginfo && !ginfo->dp ) {// non-dp symbol
		return ret && Smt::PostExp::conj( ginfo->defined_by, [&]( ruleId i )->Smt::PostExp{
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
Smt::PostExp UsableRuleOrder::_Wrapper::rule_used( ruleId i ) & {
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

static Exp::KeyValProc log_key( int& log ) {
	return Exp::OnKey("log") >> [&]( auto const& val ){
		if( log != -1 ) throw Error("#duplicate-log");
		log = TermOrder::log_of(val);
	};
}

std::unique_ptr<TermOrder> TermOrder::make(
	Exp const& x,
	std::function<Smt::Solver()> const& default_smt,
	Smt::Sort const& default_sort,
	int default_log
) {
	auto const& f = x.fun();
	uint n = 0;
	Opt<Exp> smt;
	auto mk_smt = [&]{ return smt ? Smt::Solver::of(*smt) : default_smt(); };
	int log = -1;
	auto set_log = [&]{ if( log == -1 ) log = default_log; };
	auto solver_key = Exp::OnKey("smt") >> [&]( Exp const& val ){ smt = {val}; };
	bool mono;
	auto mono_key = Exp::OnKey("mono") >> [&]( Exp const& val ){ mono = val.as_bool(); };
	if( f == "trivial" ) {
		x.process_keys(n,solver_key);
		return std::make_unique<TrivOrder>(mk_smt());
	} else if( f == "mono-sum" ) {
		x.process_keys( n, solver_key || log_key(log) );
		set_log();
		return std::make_unique<DerivedTermOrder<Poly>>("mono-sum",Poly::POS_ALGEBRA,Template::MONO_SUM,mk_smt(),true,log);
	} else if( f == "sum" ) {
		x.process_keys( n, solver_key || log_key(log) );
		set_log();
		return std::make_unique<DerivedTermOrder<Poly>>("sum",Poly::POS_ALGEBRA,Template::SUM,mk_smt(),false,log);
	} else if( f == "mat2b" ) {
		x.process_keys( n, solver_key || log_key(log) );
		set_log();
		return std::make_unique<DerivedTermOrder<TupleVal<Poly>>>(
			"mat2b", tuple_algebra<Poly::Range,Poly>({Poly::POS,Poly::POS}), Template::MAT2B, mk_smt(), false, log
		);
	} else if( f == "mat2n" ) {
		x.process_keys( n, solver_key || log_key(log) );
		set_log();
		return std::make_unique<DerivedTermOrder<TupleVal<Poly>>>(
			"mat2n", tuple_algebra<Poly::Range,Poly>({Poly::POS,Poly::POS}), Template::MAT2N, mk_smt(), false, log
		);
	} else if( f == "max" ) {
		x.process_keys( n, solver_key || log_key(log) );
		set_log();
		return std::make_unique<DerivedTermOrder<MPoly>>("max",MPoly::POS_ALGEBRA,Template::MAX,mk_smt(),false,log);
	} else if( f == "maxsum" ) {
		x.process_keys( n, solver_key || log_key(log) );
		set_log();
		return std::make_unique<DerivedTermOrder<MPoly>>("maxsum",MPoly::POS_ALGEBRA,Template::MAXSUM,mk_smt(),false,log);
	} else if( f == "imax" ) {
		x.process_keys( n, solver_key || log_key(log) );
		set_log();
		return std::make_unique<DerivedTermOrder<MPoly>>("imax",MPoly::POS_ALGEBRA,Template::IMAX,mk_smt(),false,log);
	} else if( f == "imaxsum" ) {
		x.process_keys( n, solver_key || log_key(log) );
		set_log();
		return std::make_unique<DerivedTermOrder<MPoly>>("imaxsum",MPoly::POS_ALGEBRA,Template::IMAXSUM,mk_smt(),false,log);
	} else if( f == "sum_negsum" ) {
		x.process_keys( n, solver_key || log_key(log) );
		set_log();
		return std::make_unique<DerivedTermOrder<TupleVal<MPoly>>>(
			"sum_negsum",tuple_algebra<Poly::Range,MPoly>({Poly::POS,Poly::NEG}), Template::SUM_NEGSUM, mk_smt(), false, log
		);
	} else if( f == "imaxsum_negsum" ) {
		x.process_keys( n, solver_key || log_key(log) );
		set_log();
		return std::make_unique<DerivedTermOrder<TupleVal<MPoly>>>(
			"imaxsum_negsum", tuple_algebra<Poly::Range,MPoly>({Poly::POS,Poly::NEG}), Template::IMAXSUM_NEGSUM, mk_smt(), false, log
		);
	} else if( f == "template" ) {
		std::vector<Poly::Range> rans;
		{	Exp ranexp = x.get_arg(n);
			uint j = 0;
			rans.emplace_back(Poly::range_of(ranexp.fun()));
			while( auto ranarg = ranexp.gets_arg(j) ) {
				rans.emplace_back(Poly::range_of(ranarg->as_unapplied()));
			}
		}
		Exp exp = x.get_arg(n);
		x.process_keys( n, solver_key || log_key(log) || mono_key );
		set_log();
		return std::make_unique<DerivedTermOrder<TupleVal<MPoly>>>(
			"custom-template", tuple_algebra<Poly::Range,MPoly>(std::move(rans)), exp, mk_smt(), mono, log );
	} else if( f == "path-order" ) {
		Opt<Exp> w;
		Opt<PathOrder::StatusFun> status;
		x.process_keys( n,
			Exp::OnKey("weight") >> [&]( auto const& val ){ w = {val}; } ||
			Exp::OnKey("status") >> [&]( auto const& val ){ status = {PathOrder::status_fun_of(val)}; } ||
			log_key(log) || (w ? Exp::NO_KEY : solver_key) );
		x.get_end(n);
		set_log();
		return std::make_unique<PathOrder>(
			w ?
				std::unique_ptr<TermOrder>(MemoizedTermOrder::make(make(*w,default_smt,default_sort,log))) :
				std::make_unique<TrivOrder>(mk_smt()),
			status ?
				*status :
				PathOrder::StatusFun( [](auto,auto){ return PathOrder::Status::Straight(); } ),
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
std::unique_ptr<UsableRuleOrder> UsableRuleOrder::make( std::unique_ptr<TrsOrder>&& ref, Trs const& trs, bool use ) {
	if( !use ) return std::make_unique<_Trivial>(std::move(ref));
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
	cout << lex_order([]( auto& l, auto& r ){ return l.order(r); },vector{x,y},{x,z}).ge << endl;
	cout << lex_order([]( auto& l, auto& r ){ return l.order(r); },vector{x},{x,z}).ge << endl;

	Trs::Sig sig = {{"+",{2}}};

	auto lpo = TrsOrder::make(
		std::make_unique<PathOrder>(
			std::make_unique<TrivOrder>(Smt::Z3(Smt::LIA)),
			PathOrder::StatusFun( [](auto,auto){ return PathOrder::Status::Mapped(2); } ),
			RULE
		)
	);
	lpo->extend_sig(sig,{});
	cout << lpo->rule_order(ruleId(0),{"+","x","y"},{"x"}) << endl;

}