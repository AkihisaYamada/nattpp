#include<fstream>
#include<fcntl.h>
#include<algorithm>
#include"problem.hpp"
#include"termord.hpp"
#include"deprem.hpp"
#include"graph.hpp"
#include"reach.hpp"

using namespace std;

using MSet = OrdMap<std::string,unsigned>;
using MSets = std::vector<MSet>;

ostream& operator<<( ostream& os, MSet const& X ) {
	return os << '(' << print_list(X,[]( auto const& xn ){ return Printable([&]( ostream& os )->ostream&{ return os << '(' << xn.first << ' ' << xn.second << ')'; }); }) << ')';
}
ostream& operator<<( ostream& os, MSets const& Xs ) {
	return os << '(' << print_list(Xs) << ')';
}
bool operator>=( MSet const& X, MSet const& Y ) {
	for( auto xit = X.begin(), yit = Y.begin();; xit++ ) {
		if( yit == Y.end() ) return true;
		if( xit == X.end() || xit->first > yit->first ) return false;
		if( xit->first == yit->first ) {
			if( xit->second < yit->second ) return false;
			yit++;
		}
	}
}

MSet& map_merge(
	MSet& X,
	MSet&& Y,
	std::function<unsigned&(unsigned&,unsigned)> const& f
) {
	auto xit = X.begin(), yit = Y.begin();
	for(;;) {
		if( yit == Y.end() ) {
			return X;
		}
		if( xit == X.end() ) {
			X.merge(std::move(Y));
			return X;
		}
		if( xit->first < yit->first ) {
			xit++;
		} else {
			auto&& y = Y.extract(yit++);
			if( xit->first == y.key() ) {
				f( xit->second, y.mapped() );
				xit++;
			} else {
				xit = X.insert(xit,std::move(y));
			}
		}
	}
}
MSet& operator+=( MSet& X, MSet&& Y ) {
	return map_merge(X,std::move(Y),[]( unsigned& x, unsigned y )->unsigned&{ return x += y; });
}
MSets operator+( MSets const& Xs, MSets const& Ys ) {
	MSets ret;
	for( MSet const& X : Xs ) {
		for( MSet const& Y : Ys ) {
			ret.emplace_back(X) += MSet(Y);
		}
	}
	return std::move(ret);
}

MSet& join_eq( MSet& X, MSet&& Y ) {
	return map_merge(X,std::move(Y),[]( unsigned& x, unsigned y )->unsigned&{ return x = max(x,y); });
}

// detects symbols where the same variable occurs in multiple arguments
void _max_sum_heuristic( Set<std::string>& maxsyms, Trs::Sig const& sig, Trs::Rules const& rules ) {
	Algebra<std::string,MSets> lalg =
	[&]( std::string const& f, std::vector<MSets>&& args ){
		MSets ret;
		if( sig.find(f) ) {
			if( maxsyms.contains(f) ) {
				for( auto&& arg : args ) ret.append_range(std::move(arg));
			} else {
				ret.emplace_back(MSet());
				for( auto&& arg : args ) ret = ret + arg;
			}
		} else {
			ret.emplace_back(MSet{{f,1}});
		}
		return std::move(ret);
	};
	for(;;) {
		try {
			for( auto const& [i,rule] : rules ) {
				auto const& [l,r,w] = rule;
				MSets lval = lalg(l);
				Algebra<std::string,MSets> ralg =
				[&]( std::string const& g, std::vector<MSets>&& args ){
					MSets ret;
					if( sig.find(g) ) {
						if( maxsyms.contains(g) ) {
							for( auto&& arg : args ) ret.append_range(std::move(arg));
						} else {
							ret.emplace_back(MSet());
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
						ret.emplace_back(MSet{{g,1}});
					}
					return std::move(ret);
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
	bool default_strategy = true;
	bool allow_freezing = true;
	bool use_dp = true;
	vector<Exp> rulerem_specs;
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
				{ "-freeze", { " <bool>:\tfreezing", [&]{
					require_arg();
					allow_freezing = as_bool(argv[i]);
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
					dprem_specs.push_back(Exp::of(argv[i]));
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
		dprem_specs.emplace_back("sum");
		dprem_specs.emplace_back("max");
		dprem_specs.emplace_back(LPO3_SPEC);
		dprem_specs.emplace_back("imaxsum");
		dprem_specs.emplace_back(Exp{"path-order",":weight","maxsum",":status","map"});
		dprem_specs.emplace_back("sum_negsum");
		dprem_specs.emplace_back("imaxsum_negsum");
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

	Set<std::string> heuristic;// TODO: improve
	try {
		if( p.mode == Problem::DP ) {
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
			if( allow_freezing ) {
				if( print_steps ) cerr << "; trying freezing..." << endl;
				if( p.freeze() ) {
					if( print_freezed ) {
						Trs::print_sig(*prf << "(freeze",p.main.sig,"\n  ");
						Trs::print_rules(*prf,p.main.rules,"\n  ") << ')' << endl;
					}
				}
			}
			// compute max-sum heuristic. TODO: improve
			_max_sum_heuristic(heuristic,p.main.sig,p.main.rules);
			// rule removal loop
			vector<pair<int,unique_ptr<TrsOrder>>> rule_removers;
			for( auto x : rulerem_specs ) {
				rule_removers.emplace_back(0,TrsOrder::make(TermOrder::make(x,default_smt,Smt::INT,default_log)));
			}
			auto rule_removes = [&]( auto& pair ) {
				auto& [stage,ord] = pair;
				if( print_steps ) cerr << "; trying " << ord->print_name() << "... " << endl;
				if( stage < 1 ) {// initialize for signature
					ord->extend_sig(p.main.sig,heuristic);
					stage = 1;
				}
				return order_some_rule(*ord,p.main.rules,[&](auto&&rem){
					if( print_proofs )
						*prf << "(remove-rule\n  " << ord->print(p.main.sig) << "\n " << print_list(rem) << ')' << endl;
					for( uint32_t i : rem ) {
						p.main.erase_rule(i);
					}
				});
			};
			do {
				if( p.main.rules.empty() ) throw Answer::YES;
				if( std::ranges::any_of(rule_removers,rule_removes) ) continue;
			} while(0);

			if( !use_dp ) throw Answer::MAYBE;

			if( print_proofs ) *prf << "(make_dp)" << endl;
			p.make_dps();// compute DPs
			// compute max-sum heuristic of dps. TODO: improve
			_max_sum_heuristic(heuristic,p.main.sig,p.components[0].nodes);
			if( print_maxsym ) cerr << "; max symbols: " << print_list(heuristic) << endl;
			if( print_dp ) cerr << p << endl;
			if( print_usables ) {
				cerr << "(uses_graph" << ConstGraph(p.uses_map).print_nodes("\n  ") << ')' << endl;
				cerr << "(usables" << ConstGraph(p.usable_graph).print_nodes("\n  ") << ')' << endl;
			}
			if( print_dg ) {
				cerr << "(dependency-graph" << ConstGraph(p.components.front().graph).print_nodes("\n  ") << ')' << endl;
			}
			// SCC decomposition
			p.decomp_sccs();
			if( p.components.empty() ) {
				if( print_steps ) cerr << "; no SCC" << endl;
				throw Answer::YES;
			}
			if( print_proofs ) *prf << "(scc" << p.components.front().nodes << ')' << endl;
		}
		uint32_t target_ind = 2;
		auto dp_removes = [&]( pair<int,unique_ptr<UsableRuleOrder>>& pair ){
			auto& [subcomp,subgraph] = p.components.front();
			auto& [stage,ord] = pair;
			if( print_steps ) cerr << "; trying " << ord->print_name() << "... " << endl;
			if( stage == 0 ) {
				ord->extend_sig(p.main.sig,heuristic);
				stage = 1;
			}
			return order_some_dp(*ord,p,subcomp,[&]( auto&& rem, auto const& usables ){
				if( print_proofs ) {
					*prf << "(remove-dp\n  (" << ord->print_name();
					auto pr_sym = [&]( auto const& f ) {
						*prf << "\n    (" << f << ord->print_sym_info(f) << ')';
					};
					for( auto [f,rank] : p.main.sig ) pr_sym(f);
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
				if( p.components.empty() ) throw Answer::YES;
				if( print_proofs ) *prf << "(scc" << p.components.front().nodes << ')' << endl;
				continue;
			}
			if( std::ranges::any_of(dp_removers,dp_removes) ) {
				// SCC decomposition
				p.decomp_sccs();
				if( p.components.empty() ) {
					if( print_steps ) cerr << "; no more SCC" << endl;
					throw Answer::YES;
				}
				if( print_proofs ) *prf << "(scc" << p.components.front().nodes << ')' << endl;
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
					auto usables = Set<uint32_t>();
					for( auto const& comp : p.components ) {
						for( auto const& [i,dp] : comp.nodes ) {
							for( auto const& u : **ASSERTED(p.usable_graph.find(i)) ) {
								usables.emplace(u);
							}
						}
					}
					for( auto it = p.main.rules.begin(); it != p.main.rules.end(); ) {
						if( usables.contains(it->first) ) {
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