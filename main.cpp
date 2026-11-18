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
	Opt<StrDeriver> osubst;
	Exp default_smt_spec = Exp{"z3","QF_LRA"};
	Opt<Smt::BaseSort> default_sort;
	int default_log = -1;
	Opt<uint8_t> edge_len;
	bool default_strategy = true;
	enum class FreezingMode { NONE, FULL, UNCURRY } allow_freezing = FreezingMode::UNCURRY;
	enum EdgeMode { NONE, AUTO = 1, FULL = AUTO << 1 } allow_edge = EdgeMode::AUTO;
	bool use_dp = true;
	bool use_superterm_criterion = true;
	struct RuleRemInfo { Exp exp; Opt<Exp> smt; };
	vector<RuleRemInfo> rulerem_specs;
	struct DpRemInfo { Exp exp; Opt<Exp> smt; bool usable = true; bool edge = false; };
	vector<DpRemInfo> dprem_specs;
	for( int i = 1; i < argc; i++ ) {
		if( argv[i][0] == '-' ) {
			string_view opt = argv[i];
			auto require_arg = [&]{
				i++;
				if( i == argc ) throw Error("#missing-arg",opt);
			};
			static auto print_ref = [&]( std::string const& )->bool& {
				static Map<std::string,bool&> print_map = {
					{"problem",print_problem},
					{"proof",print_proofs},
					{"step",print_steps},
					{"dg",print_dg},
					{"usable",print_usables},
					{"fail",print_on_fail},
					{"maxsum",print_maxsym},
				};
				return print_map.find(argv[i]) | [&]{ throw Error("#main","\"unknown print option\"",argv[i]); };
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
					allow_freezing = map.find(argv[i]) | [&]{ throw Error("#command","\"unknown freezing mode\"",argv[i]); };
				}}},
				{ "-edge", { " (none|auto|full):\tedge remove mode", [&]{
					require_arg();
					static Map<std::string,EdgeMode> map = {
						{"full",EdgeMode::FULL},{"auto",EdgeMode::AUTO},{"none",EdgeMode::NONE}
					};
					allow_edge = map.find(argv[i]) | [&]{ throw Error("#command","\"unknown edge remove mode\"",argv[i]); };
				}}},
				{ "-edge-len", { " <nat>:\tedge length", [&]{
					if( edge_len ) throw Error("#duplicate-option",argv[i]);
					require_arg();
					edge_len = {(uint8_t)as_nat(argv[i],[](auto i){ return i < 127; })};
				}}},
				{ "-smt", { " <exp>:\tspecify SMT", [&]{
					require_arg();
					default_smt_spec = Exp::of(argv[i]);
				}}},
				{ "-log", { " <lvl>:\tlog level", [&]{
					if( default_log != -1 ) throw Error("#duplicate-log");
					require_arg();
					default_log = TermOrder::log_of(argv[i]);
				}}},
				{ "-print", { " (problem|proof|step|dg|dp|usable|fail|maxsum):\tprint dependency graph", [&]{
					require_arg();
					print_ref(argv[i]) = true;
				}}},
				{ "-no-print", { " (problem|proof|step|dg|dp|usable|fail|maxsum):\tprint dependency graph", [&]{
					require_arg();
					print_ref(argv[i]) = false;
				}}},
				{ "-s", { "<exp>:\t strategy", [&]{
					default_strategy = false;
					require_arg();
					auto reader = Reader(argv[i]);
					while( reader.opens() ) {
						if( reader.reads_sym("rule-remover") ) {
							rulerem_specs.emplace_back(reader.read_exp());
						} else if( reader.reads_sym("dp-remover") ) {
							auto& spec = dprem_specs.emplace_back(reader.read_exp());
							for(;;) {
								if( reader.reads_key(":usable") ) {
									spec.usable = reader.read_bool();
								} else if( reader.reads_key(":edge") ) {
									spec.edge = reader.read_bool();
								} else if( reader.reads_key(":smt") ) {
									spec.smt.emplace(reader.read_exp());
								} else {
									break;
								}
							};
						} else {
							throw reader.error("#syntax","\"malformed strategy\"");
						}
						reader.close();
					}
					reader.eof();
				}}},
				{ "-dp", { ":\tcompute dependency pairs", [&]{
					use_dp = true;
					default_strategy = false;
				}}},
				{ "-super", { ":\tsuperterm criterion", [&]{
					use_superterm_criterion = true;
				}}},
				{ "-subst", { " <subst>\tapply substitution", [&]{
					require_arg();
					osubst = {StrDeriver::of(Exp::of(argv[i]))};
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
	if( default_log == -1 ) {
		default_log = NONE;
	}
	auto p = Problem( ois ? *ois : cin );
	auto prf = OStream( oprf ? *oprf : cerr );
	if( default_strategy ) {
		rulerem_specs.emplace_back("mono-sum");
		use_dp = true;
		bool auto_edge = allow_edge & (EdgeMode::AUTO | EdgeMode::FULL);
		bool full_edge = allow_edge == EdgeMode::FULL;
		dprem_specs.push_back({ .exp="sum", .edge=auto_edge });
		dprem_specs.push_back({ .exp="max", .edge=full_edge });
		dprem_specs.push_back({ .exp="path-order", .edge=false });
		dprem_specs.push_back({ .exp="imaxsum", .edge=auto_edge });
		dprem_specs.push_back({ .exp=Exp{"path-order",":weight","maxsum"}, .edge=false });
		dprem_specs.push_back({ .exp="sum_negsum", .edge=auto_edge });
		dprem_specs.push_back({ .exp="mat2b", .edge=full_edge });
		dprem_specs.push_back({ .exp="imaxsum_negsum", .edge=full_edge });
	}
	if( mode == UNSET && p.mode == Problem::NONE ) {
		p.mode = Problem::SN;
	} else if( mode == SOME ) {
		use_dp = false;
	} else if( p.mode == Problem::SAT ) {
		cout << "Unsupported!" << endl;
		exit(-1);
	} else if( osubst ) {
		cerr << *osubst << endl;
		for( auto const& [i,rule] : p.rules ) {
			auto const& [l,r,w] = rule;
			cout << "(rule " << osubst->subst(l) << ' ' << osubst->subst(r) << " :number " << i << ')' << endl;
		}
		exit(0);
	}
	if( print_problem ) cerr << p << endl;
	auto print_scc_count = print_steps ? function([&]{
		cerr << "; SCCs: " << p.components.size() << endl;
	}) : []{};

	auto print_component = print_steps || print_proofs ? function([&]{
		*prf << "(scc";
		for( auto const& [i,js] : p.components.front().graph ) {
			*prf << ' ' << i;
		}
		*prf << ')' << endl;
	}) : []{};

	auto decomp_sccs = [&]{
		if( p.decomp_sccs() ) {
			if( p.components.empty() ) {
				if( print_steps ) cerr << "; no SCC" << endl;
				throw Answer::YES;
			}
			print_scc_count();
			print_component();
			return true;
		}
		return false;
	};

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
			for( ruleId i : p.triv_rules ) {
				if( print_proofs ) *prf << "(looping-rule " << i << ')' << endl;
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
			for( auto const& [x,smt] : rulerem_specs ) {
				rule_removers.emplace_back(0,
					TrsOrder::make(
						TermOrder::make(x,[&]{ return Smt::Solver::of(smt.value_or(default_smt_spec)); },default_log)
					)
				);
			}
			auto rule_removes = [&]( auto& remover ) {
				auto& [stage,ord] = remover;
				if( print_steps ) cerr << "; trying " << ord->print_name() << "... " << endl;
				if( stage < 1 ) {// initialize for signature
					ord->extend_sig(p.sig,heuristic);
					stage = 1;
				}
				return order_some_rule(*ord,p.rules,[&](auto&&rem){
					if( print_proofs ) {
						*prf << "(remove-rule\n  " << ord->print(p.sig) << "\n " << print_list(rem) << ')' << endl;
					}
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
			auto& dg = p.components.front().graph;
			if( print_proofs ) {
				*prf << "(dps" << endl;
				for( auto const& [i,js] : dg ) {
					auto const& [l,r,w] = *ASSERTED(p.rules.find(i));
					*prf << " (dp " << l << ' ' << r << " :number " << i;
					if( print_dg ) *prf << "\n  :edges (" << print_list(js) << ')';
					*prf << ')' << endl;
				}
				*prf << ')' << endl;
			}
			if( print_usables ) {
				cerr << "(uses_graph" << ConstGraph(p.uses_map).print_nodes("\n  ") << ')' << endl;
				cerr << "(usables" << ConstGraph(p.dp_use_map).print_nodes("\n  ") << ')' << endl;
			}
			// superterm criterion
			if( use_superterm_criterion ) {
				if( print_steps ) *prf << "; trying superterm criterion..." << endl;
				vector<pair<ruleId,Set<ruleId>>> rem_edges;
				for( auto& [i,js] : dg ) {
					auto const& [l,r,w] = *ASSERTED(p.rules.find(i));
					auto const& [f,rs] = *r;
					auto n = rs.size();
					Set<ruleId> node;
					for( auto j = js.begin(); j != js.end(); ) {
						auto const& [g,ts] = *p.get_renamed_lhs(*j);
						if( f == g && ts.size() == n &&
							exists( 0, n, [&]( auto k ){ return rs[k].subterm(ts[k]); } )
						) {
							node.emplace(*j);
							j = js.erase(j);
						} else {
							j++;
						}
					}
					if( !node.empty() ) {
						rem_edges.emplace_back(i,std::move(node));
					}
				}
				if( !rem_edges.empty() ) {
					if( print_proofs ) {
						*prf << "(superterm-criterion\n ";
						for( auto const& [i,js] : rem_edges ) {
							*prf << " (edge " << i;
							for( auto j : js ) {
								*prf << ' ' << j;
							}
							*prf << ')' << endl;
						}
						*prf << ")\n";
					}
				}
			}
			// SCC decomposition
			decomp_sccs();
		}
		// compute max-sum heuristic of dps. TODO: improve
		_max_sum_heuristic(heuristic,p.sig,p.rules);
		if( print_maxsym ) cerr << "; max symbols: " << print_list(heuristic) << endl;
		struct DpRemover {
			int stage;
			bool edge;
			unique_ptr<UsableRuleOrder> order;
		};
		vector<DpRemover> dp_removers;
		for( auto spec : dprem_specs ) {
			dp_removers.emplace_back(0,spec.edge,
				UsableRuleOrder::make(
					TrsOrder::make(
						TermOrder::make(spec.exp,[&]{
							return Smt::Solver::of(spec.smt.value_or(default_smt_spec));
						},default_log)
					),p,spec.usable
				)
			);
		}

		for(;;) {
			if( p.components.front().graph.empty() ) {
				p.components.pop_front();
				if( p.components.empty() ) {
					throw Answer::YES;
				}
				print_scc_count();
				print_component();
				continue;
			}
			auto& comp = p.components.front();
			if( std::ranges::any_of( dp_removers, [&]( DpRemover& remover ){
				if( print_steps ) cerr << "; trying " << remover.order->print_name() << "... " << endl;
				if( remover.stage == 0 ) {
					remover.order->extend_sig(p.sig,heuristic);
					remover.stage = 1;
				}
				return order_some_dp( *remover.order, p,
					print_proofs ? function<void(vector<ruleId>&&,vector<ruleId>&&)>(
						[&]( auto&& remdps, auto&& usables ){
							*prf << "(remove-dp\n  " << remover.order->print(p.sig) << "\n ";
							for( auto i : remdps ) {
								*prf << ' ' << i;
								comp.graph.erase(i);
								p.erase_rule(i);
							}
							*prf << "\n  :usables (" << print_list(usables) << "))" << endl;
						}
					) : [&]( auto&& remdps, auto&& usables ){
						for( auto i : remdps ) {
							comp.graph.erase(i);
							p.erase_rule(i);
						}
					}
				);
			} ) ) {
				decomp_sccs();
				continue;
			}
			for(;;){// edge removal loop
				if( std::ranges::any_of( dp_removers, [&]( DpRemover& remover ){
					if( !remover.edge ) return false;
					if( print_steps ) cerr << "; trying edge removal by " << remover.order->print_name() << "... " << endl;
					return bound_by_edge( *remover.order, p, print_proofs ? function<void(std::vector<std::pair<ruleId,std::vector<ruleId>>>&&,std::vector<ruleId>&&)>(
						[&]( auto&& nodes, auto&& usables ){
						*prf << "(remove-edge\n  " << remover.order->print(p.sig) << flush;
						for( auto const& [i,js] : nodes ) {
							auto& tgts = comp.graph[i];
							*prf << "\n  (edge " << i;
							for( auto j : js ) {
								tgts.erase(j);
								*prf << ' ' << j;
							}
							*prf << ')' << flush;
						}
						*prf << "\n  :usables (" << print_list(usables) << "))" << endl;
					}) : [&]( auto&& nodes, auto&& usables ){
						for( auto const& [i,js] : nodes ) {
							auto& tgts = comp.graph[i];
							for( auto j : js ) {
								tgts.erase(j);
							}
						}
					});
				} ) ) {
					if( decomp_sccs() ) break;
				} else {
					throw Answer::MAYBE;
				}
			}
		}
	} catch( Answer a ) {
		if( a == Answer::YES ) {
			cout << "YES" << endl;
		} else if( a == Answer::NO ) {
			cout << "NO" << endl;
		} else if( a == Answer::MAYBE ) {
			if( print_steps ) cerr << "; failed" << endl;
			cout << "MAYBE" << endl;
			if( print_on_fail ) {
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