#include<fstream>
#include<fcntl.h>
#include<algorithm>
#include"problem.hpp"
#include"termord.hpp"
#include"deprem.hpp"
#include"graph.hpp"

using namespace std;

int main( int argc, char* argv[] ) try {
	Opt<ifstream> ois;
	Opt<ofstream> oprf;
	bool exit_on_error = false;
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
			if( opt == "-some" ) {
				if( mode != UNSET ) throw Error("#duplicate-mode",opt);
				mode = SOME;
			} else if( opt == "-sort" ) {
				if( default_sort ) throw Error("#duplicate-sort");
				require_arg();
				default_sort.emplace( Smt::BaseSort::of(argv[i]) );
			} else if( opt == "-proof" ) {
				if( oprf ) throw Error("#duplicate-proof");
				require_arg();
				oprf.emplace(argv[i]);
			} else if( opt == "-smt" ) {
				require_arg();
				default_smt_spec = {Exp::of(argv[i])};
			} else if( opt == "-log" ) {
				if( default_log != -1 ) throw Error("#duplicate-log");
				require_arg();
				default_log = TermOrder::log_of(argv[i]);
			} else if( opt == "-a" ) {// general remover
				require_arg();
				rem_specs.push_back(Exp::of(argv[i]));
				default_strategy = false;
				use_dp = true;
				use_marked_dprem = true;
			} else if( opt == "-r" ) {// rule remover
				require_arg();
				rulerem_specs.push_back(Exp::of(argv[i]));
				default_strategy = false;
			} else if( opt == "-d" ) {// dp remover
				require_arg();
				dprem_specs.push_back(Exp::of(argv[i]));
				default_strategy = false;
				use_dp = true;
			} else if( opt == "-udp" ) {// unmarked dps only
				use_dp = true;
				use_unmarked_dprem = true;
			} else if( opt == "-umdp" ) {// unmarked and marked dps
				use_dp = true;
				use_unmarked_dprem = true;
			} else if( opt == "-mdp" ) {// marked dps only
				use_dp = true;
				use_unmarked_dprem = false;
				use_marked_dprem = true;
			} else {
				throw Error("#unknown-option",argv[i]);
			}
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
		default_sort = {Smt::INT};
	}
	auto default_smt = [&]()->Smt::Solver{
		if( default_smt_spec ) {
			return Smt::Solver::of(*default_smt_spec);
		} else {
			return Smt::Z3( *default_sort == Smt::REAL ? Smt::QF_LRA : Smt::QF_LIA );
		}
	};
	if( default_log == -1 ) {
		default_log = TermOrder::NONE;
	}
	auto p = Problem( ois ? *ois : cin );
	cerr << p << endl;
	auto const& [sig,rules] = p.systems.front();
	switch( mode ) {
	case UNSET: case SN:
		if( default_strategy ) {
			rulerem_specs.emplace_back("mono-sum");
			dprem_specs.emplace_back("sum");
			dprem_specs.emplace_back("max");
			dprem_specs.emplace_back(LPO3_SPEC);
			dprem_specs.emplace_back(Exp{"path-order",":weight","max",":status","map"});
			use_dp = true;
		}
		break;
	case SOME:
		use_dp = false;
		break;
	}

	if( auto it = p.extra_var.begin(); it != p.extra_var.end() ) {
		auto const& [no,var] = *it;
		cerr << "(extra-var " << var << " :rule " << no << ')' << endl;
		throw Answer::NO;
	}
	vector<pair<int,unique_ptr<TrsOrder>>> rule_removers;
	for( auto x : rulerem_specs ) {
		rule_removers.emplace_back(0,TrsOrder::of(TermOrder::of(x,default_smt,Smt::INT,default_log)));
	}
	vector<pair<int,unique_ptr<TrsOrder>>> both_removers;
	for( auto x : rem_specs ) {
		both_removers.emplace_back(0,TrsOrder::of(TermOrder::of(x,default_smt,Smt::INT,default_log)));
	}

	try {
		Problem::SysIt target = p.systems.begin();
		auto& [sig,rules] = *target;

		// rule removal loop
		auto rule_removes = [&]( pair<int,unique_ptr<TrsOrder>>& pair ) {
			auto& [stage,ord] = pair;
			cerr << "; trying " << ord->print_name() << "... " << endl;
			if( stage < 1 ) {// initialize for signature
				ord->extend_sig(sig);
				stage = 1;
			}
			return order_some_rule(*ord,rules,[&](auto&&rem){
				cerr << "(remove-rule\n  " << ord->print(sig) << "\n ";
				for( size_t i : rem ) {
					cerr << ' ' << i;
					p.systems.front().rules.erase(i);
				}
				cerr << ")" << endl;
			});
		};
		do {
			if( rules.empty() ) throw Answer::YES;
		} while(
			std::ranges::any_of(rule_removers,rule_removes) ||
			std::ranges::any_of(both_removers,rule_removes)
		);

		if( !use_dp ) throw Answer::MAYBE;

		cerr << "; taking DPs" << endl;
		p.make_dps();// compute DPs
		target++;
		// SCC decomposition
		auto dps = Trs::Rules();
		swap(dps,target->rules);
		Graph dg;
		for( auto const& [i,dp] : dps ) {
			auto const& l = dp.first, &r = dp.second;
			dg.map.emplace(i,ASSERTED(sig.find(r.fun()))->depends);
		}
		for( auto const& scc : dg.sccs() ) {
			auto& back = p.systems.emplace_back();
			for( auto const& i : scc ) {
				back.rules.emplace(i,*ASSERTED(dps.find(i)));
			}
		}
		size_t target_ind = 1;
		bool marked = false;

		auto dp_removes = [&]( pair<int,unique_ptr<TrsOrder>>& pair ){
			auto& [subsig,subcomp] = *target;
			auto& [stage,ord] = pair;
			cerr << "; trying " << ord->print_name() << "... " << endl;
			if( stage == 0 ) {
				ord->extend_sig(sig);
				stage = 1;
			}
			if( marked && stage < target_ind ) {
				ord->extend_sig(subsig);
				stage = target_ind;
			}
			return order_some_dp(*ord,rules,subcomp,[&]( auto&& rem ){
				cerr << "(remove-dp\n  (" << ord->print_name();
				auto pr_sym = [&]( auto const& f ) {
					cerr << "\n    (" << f << ' ' << ord->print_sym_info(f) << ')';
				};
				for( auto [f,rank] : sig ) pr_sym(f);
				for( auto [f,rank] : subsig ) pr_sym(f);
				cerr << ")\n  ";
				for( size_t i : rem ) {
					cerr << ' ' << i;
					subcomp.erase(i);
				}
				cerr << ')' << endl;
			});
		};

		vector<pair<int,unique_ptr<TrsOrder>>> dp_removers;
		for( auto x : dprem_specs ) {
			dp_removers.emplace_back(0,TrsOrder::of(TermOrder::of(x,default_smt,Smt::INT,default_log)));
		}
		for(;;) {
			if( target->rules.empty() ) {
				target++;
				if( target == p.systems.end() ) throw Answer::YES;
				target_ind++;
				marked = false;
				cerr << "(scc" << target->rules << ')' << endl;
				continue;
			}
			if( !marked ) {
				if( use_unmarked_dprem ) {
					if( std::ranges::any_of(both_removers,dp_removes) ) continue;
					if( std::ranges::any_of(dp_removers,dp_removes) ) continue;
				}
				if( use_marked_dprem ) {
					p.mark_dps(target);
					cerr << "(mark_dp" << target->rules << ')' << endl;
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
			exit(0);
		} else if( a == Answer::NO ) {
			cout << "NO" << endl;
			exit(1);
		} else if( a == Answer::MAYBE ) {
			cerr << p << endl;
			cout << "MAYBE" << endl;
			exit(2);
		} else {
			assert(false);
		}
	} 
} catch( Error const& e ) {
	cerr << e << endl;
	exit(-1);
} catch( std::exception const& e ) {
	cerr << e.what() << endl;
	exit(-1);
}