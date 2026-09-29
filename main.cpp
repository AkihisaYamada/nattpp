#include<fstream>
#include<fcntl.h>
#include<algorithm>
#include"problem.hpp"
#include"termord.hpp"
#include"deprem.hpp"
#include"graph.hpp"
#include"reach.hpp"

using namespace std;

int main( int argc, char* argv[] ) try {
	Opt<ifstream> ois;
	Opt<ofstream> oprf;
	bool exit_on_error = false;
	bool print_problem = true;
	bool print_steps = true;
	bool print_proofs = true;
	bool print_dp = false;
	bool print_dg = false;
	bool print_usables = false;
	bool print_on_fail = false;
	enum { UNSET, SN, SOME } mode = UNSET;
	Opt<Exp> default_smt_spec;
	Opt<Smt::BaseSort> default_sort;
	int default_log = -1;
	bool default_strategy = true;
	bool use_dp = true;
	bool use_unmarked_dprem = false;
	bool use_marked_dprem = true;
	vector<Exp> rulerem_specs;
	vector<Exp> rem_specs;
	vector<Exp> dprem_specs;
	for( int i = 1; i < argc; i++ ) {
		if( argv[i][0] == '-' ) {
			string_view opt = argv[i];
			auto require_arg = [&]{
				i++;
				if( i == argc ) throw Error("#missing-arg",opt);
			};
			OrdMap<std::string,std::pair<std::string,std::function<void(void)>>> map = {
				{ "-q", { ":\tquiet mode", [&]{
					print_problem = print_proofs = print_steps = print_dp = print_on_fail = false;
				}}},
				{ "-order-some", { ":\torder some rule", [&]{
					if( mode != UNSET ) throw Error("#duplicate-mode",opt);
					mode = SOME;
				}}},
				{ "-sort", { " <sort>:\tspecify default SMT sort", [&]{
					if( default_sort ) throw Error("#duplicate-sort");
					require_arg();
					default_sort.emplace( Smt::BaseSort::of(argv[i]) );
				}}},
				{ "-proof", { " <out>:\tspecify proof output", [&]{
					if( oprf ) throw Error("#duplicate-proof");
					require_arg();
					oprf.emplace(argv[i]);
				}}},
				{ "-smt", { " <exp>:\tspecify SMT", [&]{
					require_arg();
					default_smt_spec = {Exp::of(argv[i])};
				}}},
				{ "-log", { " <lvl>:\tlog level", [&]{
					if( default_log != -1 ) throw Error("#duplicate-log");
					require_arg();
					default_log = TermOrder::log_of(argv[i]);
				}}},
				{ "-print-on-fail", { ":\tprint remaining problem on unsuccessful proof", [&]{
					print_on_fail = true;
				}}},
				{ "-print-dp", { ":\tprint dependency pairs", [&]{
					print_dp = true;
				}}},
				{ "-print-dg", { ":\tprint dependency graph", [&]{
					print_dg = true;
				}}},
				{ "-print-usables", { ":\tprint usable rules", [&]{
					print_usables = true;
				}}},
				{ "-a" , { " <ord>:\tgeneral remover", [&]{
					require_arg();
					rem_specs.push_back(Exp::of(argv[i]));
					default_strategy = false;
					use_dp = true;
					use_marked_dprem = true;
				}}},
				{ "-r", { " <ord>:\trule remover", [&]{
					require_arg();
					rulerem_specs.push_back(Exp::of(argv[i]));
					default_strategy = false;
				}}},
				{ "-d", { " <ord>:\tdp remover", [&]{
					require_arg();
					dprem_specs.push_back(Exp::of(argv[i]));
					default_strategy = false;
					use_dp = true;
				}}},
				{ "-udp", { ":\tunmarked dps only", [&]{
					use_dp = true;
					use_unmarked_dprem = true;
					use_marked_dprem = false;
					default_strategy = false;
				}}},
				{ "-umdp", { ":\tunmarked and marked dps", [&]{
					use_dp = true;
					use_unmarked_dprem = true;
					default_strategy = false;
				}}},
				{ "-mdp", { ":\tmarked dps only", [&]{
					use_dp = true;
					use_unmarked_dprem = false;
					use_marked_dprem = true;
					default_strategy = false;
				}}},
				{ "-h", { ":\tshow this help", [&]{
					cout << "usage: " << argv[0] << " [<option>...] [<input>]" << endl << "options:" << endl;
					for( auto const& [opt,body] : map ) {
						auto const& [help,fun] = body;
						cout << "  " << opt << help << endl;
					}
					exit(0);
				}}}
			};
			auto const& body = map.find(opt);
			if( !body ) {
				throw Error("#unknown-option",argv[i]);
			}
			body->second();
		} else {
			if( ois ) throw Error("#too-many-arguments",argv[i]);
			ois.emplace(argv[i]);
			if( ois->fail() ) {
				throw Error("#open-failed",argv[i]);
			}
			exit_on_error = true;
		}
	}
	if( !default_sort ) {
		default_sort = {Smt::REAL};
	}
	auto default_smt = [&]()->Smt::Solver{
		if( default_smt_spec ) {
			return Smt::Solver::of(*default_smt_spec);
		} else {
			return Smt::Z3( *default_sort == Smt::REAL ? Smt::QF_LRA : Smt::QF_LIA );
		}
	};
	if( default_log == -1 ) {
		default_log = NONE;
	}
	auto p = Problem( ois ? *ois : cin );
	auto prf = OStream( oprf ? *oprf : cerr );
	if( print_problem ) cerr << p << endl;
	if( default_strategy ) {
		rulerem_specs.emplace_back("mono-sum");
		dprem_specs.emplace_back("sum");
		dprem_specs.emplace_back("max");
		dprem_specs.emplace_back(LPO3_SPEC);
		dprem_specs.emplace_back(Exp{"path-order",":weight","max",":status","map"});
		dprem_specs.emplace_back("imax");
		dprem_specs.emplace_back("mat2b");
		use_dp = true;
	}
	if( mode == UNSET && p.mode == Problem::NONE ) {
		p.mode = Problem::SN;
	} else if( mode == SOME ) {
		use_dp = false;
	} else if( p.mode == Problem::SAT ) {
		for( auto const& component : p.components ) {
			for( auto const& [i,pair] : component.nodes ) {
				if( may_reach(p.main,pair.first,pair.second,8,true) ) {
					cout << "unknown" << endl;
					exit(1);
				}
			}
		}
		cout << "unsat" << endl;
		exit(0);
	}

	try {
		vector<pair<int,unique_ptr<TrsOrder>>> rule_removers;
		for( auto x : rulerem_specs ) {
			rule_removers.emplace_back(0,TrsOrder::make(TermOrder::make(x,default_smt,Smt::INT,default_log)));
		}
		vector<pair<int,unique_ptr<UsableRuleOrder>>> both_removers;
		for( auto x : rem_specs ) {
			both_removers.emplace_back(0,UsableRuleOrder::make_triv(TrsOrder::make(TermOrder::make(x,default_smt,Smt::INT,default_log))));
		}
		bool marked = false;
		if( p.mode == Problem::DP ) {
			use_unmarked_dprem = false;
			marked = true;
			p.init_uses();
			if( print_usables ) {
				cerr << "(uses_graph" << ConstGraph(p.uses_map).print_nodes("\n  ") << ')' << endl;
				cerr << "(usables" << ConstGraph(p.usable_graph).print_nodes("\n  ") << ')' << endl;
			}
		} else {
			for( auto const& [no,var] : p.extra_var ) {
				if( print_proofs ) *prf << "(extra-var " << var << " :rule " << no << ')' << endl;
				throw Answer::NO;
			}
			// rule removal loop
			auto rule_removes = [&]( auto& pair ) {
				auto& [stage,ord] = pair;
				if( print_steps ) cerr << "; trying " << ord->print_name() << "... " << endl;
				if( stage < 1 ) {// initialize for signature
					ord->extend_sig(p.main.sig);
					stage = 1;
				}
				return order_some_rule(*ord,p.main.rules,[&](auto&&rem){
					if( print_proofs )
						*prf << "(remove-rule\n  " << ord->print(p.main.sig) << "\n " << print_list(rem) << ')' << endl;
					for( uint32_t i : rem ) {
						p.main.rules.erase(i);
					}
				});
			};
			do {
				if( p.main.rules.empty() ) throw Answer::YES;
				if( std::ranges::any_of(rule_removers,rule_removes) ) continue;
				if( std::ranges::any_of(both_removers,rule_removes) ) continue;
			} while(0);

			if( !use_dp ) throw Answer::MAYBE;

			if( print_steps ) cerr << "; taking DPs" << endl;
			p.make_dps();// compute DPs
			if( print_proofs ) *prf << "(make_dp)" << endl;
			if( print_dp ) cerr << p << endl;
			if( print_usables ) {
				cerr << "(uses_graph" << ConstGraph(p.uses_map).print_nodes("\n  ") << ')' << endl;
				cerr << "(usables" << ConstGraph(p.usable_graph).print_nodes("\n  ") << ')' << endl;
			}
			// SCC decomposition
			auto dps = std::move(p.components.front().nodes);
			p.components.pop_front();
			Graph dg = [&]{
				Map<uint32_t,Set<uint32_t>> dgmap;
				for( auto const& [i,dp] : dps ) {
					auto const& [l1,r1,w] = dp;
					auto [nexts,fl] = dgmap.emplace(i,Set<uint32_t>{});
					for( auto const& j : ASSERTED(p.main.sig.find(r1.fun()))->depends ) {
						auto const& [l2,r2,w2] = *ASSERTED(dps.find(j));
						if( may_reach(p.main,r1,l2,8,false) ) {
							nexts.emplace(j);
						}
					}
				}
				return std::move(dgmap);
			}();
			if( print_dg ) cerr << "(dependency-graph" << dg.print_nodes("\n  ") << ')' << endl;
			auto sccs = dg.sccs();
			for( uint32_t i = 0; i < sccs.size(); i++ ) {
				if( auto const& nodes = sccs[i].ref<Set<uint32_t>>() ) {// nontrivial SCCs
					auto& back = p.components.emplace_back();
					for( auto const& dp : *nodes ) {
						back.nodes.emplace(dp,*ASSERTED(dps.find(dp)));
						auto& nexts = back.graph.emplace(dp,Set<uint32_t>{}).first;
						dg.iter_nexts( dp, [&]( uint32_t next ){// copy edges inside SCC
							if( dg.scc_ind(next) == i ) {
								nexts.emplace(next);
							}
						});
					}
				}
			}
			if( p.components.empty() ) {
				if( print_steps ) cerr << "; no SCC" << endl;
				throw Answer::YES;
			}
			if( print_proofs ) *prf << "(scc" << p.components.front().nodes << ')' << endl;
		}
		uint32_t target_ind = 2;
		auto dp_removes = [&]( pair<int,unique_ptr<UsableRuleOrder>>& pair ){
			auto& [subsig,subcomp,subgraph] = p.components.front();
			auto& [stage,ord] = pair;
			if( print_steps ) cerr << "; trying " << ord->print_name() << "... " << endl;
			if( stage == 0 ) {
				ord->extend_sig(p.main.sig);
				stage = 1;
			}
			if( marked && stage < target_ind ) {
				ord->extend_sig(subsig);
				stage = target_ind;
			}
			return order_some_dp(*ord,p,subcomp,[&]( auto&& rem, auto const& usables ){
				if( print_proofs ) {
					*prf << "(remove-dp\n  (" << ord->print_name();
					auto pr_sym = [&]( auto const& f ) {
						*prf << "\n    (" << f << ord->print_sym_info(f) << ')';
					};
					for( auto [f,rank] : p.main.sig ) pr_sym(f);
					for( auto [f,rank] : subsig ) pr_sym(f);
					*prf << ")\n  " << print_list(rem) << "\n  :usables (" << print_list(usables) << "))" << endl;
				}
				for( uint32_t i : rem ) {
					subcomp.erase(i);
				}
			});
		};

		vector<pair<int,unique_ptr<UsableRuleOrder>>> dp_removers;
		for( auto x : dprem_specs ) {
			dp_removers.emplace_back(0,UsableRuleOrder::make(p.main,TrsOrder::make(TermOrder::make(x,default_smt,Smt::INT,default_log))));
		}
		for(;;) {
			if( p.components.front().nodes.empty() ) {
				p.components.pop_front();
				target_ind++;
				marked = false;
				if( p.components.empty() ) throw Answer::YES;
				if( print_proofs ) *prf << "(scc" << p.components.front().nodes << ')' << endl;
				continue;
			}
			if( !marked ) {
				if( use_unmarked_dprem ) {
					if( std::ranges::any_of(both_removers,dp_removes) ) continue;
					if( std::ranges::any_of(dp_removers,dp_removes) ) continue;
				}
				if( use_marked_dprem ) {
					p.mark_dps();
					if( print_proofs ) *prf << "(mark_dp" << p.components.front().nodes << ')' << endl;
					marked = true;
					continue;
				}
			} else {
				if( std::ranges::any_of(both_removers,dp_removes) ) continue;
				if( std::ranges::any_of(dp_removers,dp_removes) ) continue;
			}
			throw Answer::MAYBE;
		}
	} catch( Answer a ) {
		if( a == Answer::YES ) {
			cout << "YES" << endl;
		} else if( a == Answer::NO ) {
			cout << "NO" << endl;
		} else if( a == Answer::MAYBE ) {
			cout << "MAYBE" << endl;
			if( print_on_fail ) {
				if( p.mode == Problem::DP ) {
					auto usables = Set<uint32_t>();
					for( auto const& comp : p.components ) {
						for( auto const& [i,dp] : comp.nodes ) {
							for( auto const& u : **ASSERTED(p.usable_graph.find(i)) ) {
								usables.emplace(u);
							}
						}
					}
					for( auto it = p.main.rules.begin(); it != p.main.rules.end(); ) {
						if( usables.find(it->first) ) {
							it++;
						} else {
							it = p.main.rules.erase(it);
						}
					}
				}
				cerr << p << endl;
			}
		} else {
			assert(false);
		}
		exit(0);
	} 
} catch( Error const& e ) {
	cerr << e << endl;
	exit(-1);
} catch( std::exception const& e ) {
	cerr << e.what() << endl;
	exit(-1);
}