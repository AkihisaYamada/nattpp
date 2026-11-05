#include<fstream>
#include<fcntl.h>
#include<algorithm>
#include"problem.hpp"
#include"termord.hpp"
#include"deprem.hpp"
#include"graph.hpp"
#include"mset.hpp"

using namespace std;


// detects symbols where the same variable occurs in multiple arguments
void _max_sum_heuristic( Set<std::string>& maxsyms, Trs::Sig const& sig, Trs::Rules const& rules ) {
	Algebra<std::string,MSets<std::string>> lalg =
	[&]( std::string const& f, std::vector<MSets<std::string>>&& args ){
		MSets<std::string> ret;
		if( sig.find(f) ) {
			if( maxsyms.contains(f) ) {
				for( auto&& arg : args ) ret.append_range(std::move(arg));
			} else {
				ret.emplace_back(MSet<std::string>());
				for( auto&& arg : args ) ret = ret + arg;
			}
		} else {
			ret.emplace_back(MSet(f));
		}
		return ret;
	};
	for(;;) {
		try {
			for( auto const& [i,rule] : rules ) {
				auto const& [l,r,w] = rule;
				MSets<std::string> lval = lalg(l);
				Algebra<std::string,MSets<std::string>> ralg =
				[&]( std::string const& g, std::vector<MSets<std::string>>&& args ){
					MSets<std::string> ret;
					if( sig.find(g) ) {
						if( maxsyms.contains(g) ) {
							for( auto&& arg : args ) ret.append_range(std::move(arg));
						} else {
							ret.emplace_back(MSet<std::string>());
							for( auto&& arg : args ) {
								ret = ret + arg;
							}
							for( auto const& Y : ret ) {
								for( auto it = lval.begin();; it++ ) {
									if( it == lval.end() ) {// no lhs candidate can cover the rhs candidate
										maxsyms.emplace(g);
										throw false;
									}
									if( *it >= Y ) break;// this rhs candidate is covered
								}
							}
						}
					} else {
						ret.emplace_back(MSet(g));
					}
					return ret;
				};
				ralg(r);
			}
		} catch( bool _ ) {
			continue;
		}
		break;
	}
}


int main( int argc, char* argv[] ) try {
	Opt<ifstream> ois;
	Opt<ofstream> oprf;
	bool exit_on_error = false;
	bool print_problem = true;
	bool print_steps = true;
	bool print_proofs = true;
	bool print_freezed = true;
	bool print_maxsym = false;
	bool print_dp = false;
	bool print_dg = false;
	bool print_usables = false;
	bool print_on_fail = false;
	enum { UNSET, SN, SOME } mode = UNSET;
	Opt<Exp> default_smt_spec;
	Opt<Smt::BaseSort> default_sort;
	int default_log = -1;
	Opt<uint8_t> edge_len;
	bool default_strategy = true;
	enum class FreezingMode { NONE, FULL, UNCURRY } allow_freezing = FreezingMode::UNCURRY;
	enum class EdgeMode { NONE, AUTO, FULL } allow_edge = EdgeMode::AUTO;
	bool use_dp = true;
	vector<Exp> rulerem_specs;
	vector<std::pair<Exp,bool>> dprem_specs;
	for( int i = 1; i < argc; i++ ) {
		if( argv[i][0] == '-' ) {
			string_view opt = argv[i];
			auto require_arg = [&]{
				i++;
				if( i == argc ) throw Error("#missing-arg",opt);
			};
			OrdMap<std::string,std::pair<std::string,std::function<void(void)>>> map = {
				{ "-q", { ":\tquiet mode", [&]{
					print_problem = print_proofs = print_steps = print_freezed = print_dp = print_on_fail = false;
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
				{ "-freeze", { " (full|uncurry|none):\tfreezing mode", [&]{
					require_arg();
					static Map<std::string,FreezingMode> map = {
						{"full",FreezingMode::FULL},{"uncurry",FreezingMode::UNCURRY},{"none",FreezingMode::NONE}
					};
					allow_freezing = map.find(argv[i]).value_or_throw(Error("#command","\"unknown freezing mode\"",argv[i]));
				}}},
				{ "-edge", { " (full|auto|none):\tedge remove mode", [&]{
					require_arg();
					static Map<std::string,EdgeMode> map = {
						{"full",EdgeMode::FULL},{"auto",EdgeMode::AUTO},{"none",EdgeMode::NONE}
					};
					allow_edge = map.find(argv[i]).value_or_throw(Error("#command","\"unknown edge remove mode\"",argv[i]));
				}}},
				{ "-edge-len", { " <nat>:\tedge length", [&]{
					if( edge_len ) throw Error("#duplicate-option",argv[i]);
					require_arg();
					edge_len = {(uint8_t)as_nat(argv[i],[](auto i){ return i < 127; })};
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
				{ "-print-maxsum", { ":\tprint max-sum heuristic", [&]{
					print_maxsym = true;
				}}},
				{ "-r", { " <ord>:\trule remover", [&]{
					require_arg();
					rulerem_specs.push_back(Exp::of(argv[i]));
					default_strategy = false;
				}}},
				{ "-d", { " <ord>:\tdependency pair remover", [&]{
					require_arg();
					dprem_specs.push_back({Exp::of(argv[i]),false});
					default_strategy = false;
					use_dp = true;
				}}},
				{ "-dp", { ":\tcompute dependency pairs", [&]{
					use_dp = true;
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
		dprem_specs.emplace_back( "sum", allow_edge != EdgeMode::NONE );
		dprem_specs.emplace_back( "max", allow_edge == EdgeMode::FULL );
		dprem_specs.emplace_back( LPO3_SPEC, allow_edge != EdgeMode::NONE );
		dprem_specs.emplace_back( "imaxsum", allow_edge != EdgeMode::FULL );
		dprem_specs.emplace_back( Exp{"path-order",":weight","maxsum",":status","map"}, allow_edge != EdgeMode::FULL );
		dprem_specs.emplace_back( "sum_negsum", allow_edge != EdgeMode::NONE );
		dprem_specs.emplace_back( "mat2b", allow_edge != EdgeMode::FULL );
		dprem_specs.emplace_back( "imaxsum_negsum", allow_edge != EdgeMode::FULL );
		use_dp = true;
	}
	if( mode == UNSET && p.mode == Problem::NONE ) {
		p.mode = Problem::SN;
	} else if( mode == SOME ) {
		use_dp = false;
	} else if( p.mode == Problem::SAT ) {
		cout << "Unsupported!" << endl;
		exit(-1);
	}

	auto print_component = print_proofs ? function([&]{
		*prf << "(scc";
		for( auto const& [i,js] : p.components.front().graph ) {
			*prf << endl << " (node " << i << " (" << print_list(js) << "))";
		}
		*prf << ')' << endl;
	}) : []{};

	Set<std::string> heuristic;// TODO: improve
	try {
		if( p.mode == Problem::DP ) {
			p.init_uses();
			if( print_usables ) {
				cerr << "(uses_graph" << ConstGraph(p.uses_map).print_nodes("\n  ") << ')' << endl;
				cerr << "(usables" << ConstGraph(p.dp_use_map).print_nodes("\n  ") << ')' << endl;
			}
		} else {
			for( auto const& [no,var] : p.extra_var ) {
				if( print_proofs ) *prf << "(extra-var " << var << " :rule " << no << ')' << endl;
				throw Answer::NO;
			}
			if( allow_freezing != FreezingMode::NONE ) {
				if( print_steps ) cerr << "; trying freezing..." << endl;
				if( p.freeze( allow_freezing == FreezingMode::UNCURRY ) ) {
					if( print_freezed ) {
						Trs::print_sig(*prf << "(freeze",p.sig,"\n  ");
						Trs::print_rules(*prf,p.rules,"\n  ") << ')' << endl;
					}
				}
			}
			// compute max-sum heuristic. TODO: improve
//			_max_sum_heuristic(heuristic,p.sig,p.rules);
			// rule removal loop
			vector<pair<int,unique_ptr<TrsOrder>>> rule_removers;
			for( auto x : rulerem_specs ) {
				rule_removers.emplace_back(0,TrsOrder::make(TermOrder::make(x,default_smt,Smt::INT,default_log)));
			}
			auto rule_removes = [&]( auto& pair ) {
				auto& [stage,ord] = pair;
				if( print_steps ) cerr << "; trying " << ord->print_name() << "... " << endl;
				if( stage < 1 ) {// initialize for signature
					ord->extend_sig(p.sig,heuristic);
					stage = 1;
				}
				return order_some_rule(*ord,p.rules,[&](auto&&rem){
					if( print_proofs )
						*prf << "(remove-rule\n  " << ord->print(p.sig) << "\n " << print_list(rem) << ')' << endl;
					for( ruleId i : rem ) {
						p.erase_rule(i);
					}
				});
			};
			do {
				if( p.rules.empty() ) throw Answer::YES;
				if( std::ranges::any_of(rule_removers,rule_removes) ) continue;
			} while(0);

			if( !use_dp ) throw Answer::MAYBE;

			if( print_steps ) cerr << "; taking dependency pairs" << endl;
			p.make_dps(edge_len.value_or(8));// compute DPs
			if( print_proofs ) {
				*prf << "(dps" << endl;
				for( auto [i,js] : p.components.front().graph ) {
					auto [l,r,w] = *ASSERTED(p.rules.find(i));
					*prf << " (dp " << l << ' ' << r << " :number " << i;
					if( print_dg ) *prf << "\n  :edges (" << print_list(js) << ')';
					*prf << ')' << endl;
				}
				*prf << ')' << endl;
			}
			// compute max-sum heuristic of dps. TODO: improve
			_max_sum_heuristic(heuristic,p.sig,p.rules);
//			if( print_maxsym ) cerr << "; max symbols: " << print_list(heuristic) << endl;
			if( print_usables ) {
				cerr << "(uses_graph" << ConstGraph(p.uses_map).print_nodes("\n  ") << ')' << endl;
				cerr << "(usables" << ConstGraph(p.dp_use_map).print_nodes("\n  ") << ')' << endl;
			}
			// SCC decomposition
			p.decomp_sccs();
			if( p.components.empty() ) {
				if( print_steps ) cerr << "; no SCC" << endl;
				throw Answer::YES;
			}
			print_component();
		}
		uint32_t target_ind = 2;
		struct DpRemover {
			int stage;
			bool edge;
			unique_ptr<UsableRuleOrder> order;
		};
		auto dp_removes = [&]( DpRemover& remover ){
			auto& comp = p.components.front();
			if( print_steps ) cerr << "; trying " << remover.order->print_name() << "... " << endl;
			if( remover.stage == 0 ) {
				remover.order->extend_sig(p.sig,heuristic);
				remover.stage = 1;
			}
			return order_some_dp_or_edge( *remover.order, p, remover.edge, [&]( auto&& rem, auto&& usables ){
				if( print_proofs ) {
					*prf << "(remove-dp\n  (" << remover.order->print_name();
					for( auto [f,rank] : p.sig ) {
						*prf << "\n    (" << f << remover.order->print_sym_info(f) << ')';
					};
					*prf << ")\n ";
				}
				for( auto [i,js] : rem ) {
					if( js.empty() ) {
						comp.graph.erase(i);
						if( print_proofs ) *prf << ' ' << i;
					} else {
						auto& tgts = comp.graph[i];
						auto itr = print_proofs ?
							*prf << " (edges " << i,
							function([&]( ruleId j ){
								tgts.erase(j);
								*prf << ' ' << j;
							}) :
							[&]( ruleId j ) {
								tgts.erase(j);
							};
						for( auto j : js ) {
							itr(j);
						}
						if( print_proofs ) *prf << ')';
					}
				}
				if( print_proofs ) *prf << "\n  :usables (" << print_list(usables) << "))" << endl;
			});
		};

		vector<DpRemover> dp_removers;
		for( auto [x,edge] : dprem_specs ) {
			dp_removers.emplace_back(0,edge,UsableRuleOrder::make(p,TrsOrder::make(TermOrder::make(x,default_smt,Smt::INT,default_log))));
		}
		for(;;) {
			if( p.components.front().graph.empty() ) {
				p.components.pop_front();
				target_ind++;
				if( p.components.empty() ) throw Answer::YES;
				print_component();
				continue;
			}
			if( std::ranges::any_of(dp_removers,dp_removes) ) {
				// SCC decomposition
				p.decomp_sccs();
				if( p.components.empty() ) {
					if( print_steps ) cerr << "; no more SCC" << endl;
					throw Answer::YES;
				}
				print_component();
				continue;
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
					auto usables = Set<ruleId>();
					for( auto const& comp : p.components ) {
						for( auto const& [i,dp] : comp.graph ) {
							for( auto const& u : **ASSERTED(p.dp_use_map.find(i)) ) {
								usables.emplace(u);
							}
						}
					}
					for( auto it = p.rules.begin(); it != p.rules.end(); ) {
						if( usables.contains(it->first) ) {
							it++;
						} else {
							it = p.rules.erase(it);
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