#include<fstream>
#include<fcntl.h>
#include"problem.hpp"
#include"termord.hpp"
#include"deprem.hpp"

using namespace std;

enum class Ans { YES, NO, MAYBE };

int main( int argc, char* argv[] ) try {
	Opt<ifstream> ois;
	Opt<ofstream> oprf;
	bool exit_on_error = false;
	enum { UNSET, SN, SOME } mode = UNSET;
	Opt<Exp> default_smt_spec;
	vector<Exp> rulerem_specs;
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
			} else if( opt == "-proof" ) {
				if( oprf ) throw Error("#duplicate-option",opt);
				require_arg();
				oprf.emplace(argv[i]);
			} else if( opt == "-smt" ) {
				require_arg();
				default_smt_spec = {Exp::of(argv[i])};
			} else if( opt == "-r" ) {// rule remover
				require_arg();
				rulerem_specs.push_back(Exp::of(argv[i]));
			} else if( opt == "-d" ) {// dp remover
				require_arg();
				dprem_specs.push_back(Exp::of(argv[i]));
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
	auto default_smt = [&]()->Smt::Solver{
		if( default_smt_spec ) {
			return Smt::Solver::of(*default_smt_spec);
		} else {
			return Smt::Z3(Smt::QF_LIA);
		}
	};
	auto p = Problem( ois ? *ois : cin );
	cout << p << endl;
	auto const& sig = p.sig;
	auto const& trs = p.systems[0];
	bool mono;
	bool use_dp;
	switch( mode ) {
	case UNSET: case SN:
		mono = true;
		if( dprem_specs.empty() ) {
			if( rulerem_specs.empty() ) {// default strategy
				rulerem_specs.emplace_back(SUM_SPEC);
				dprem_specs.emplace_back(SUM_SPEC);
				dprem_specs.emplace_back(LPO3_SPEC);
				use_dp = true;
			} else {
				use_dp = false;
			}
		} else {
			use_dp = true;
		}
		break;
	case SOME:
		mono = false;
		use_dp = false;
		break;
	}
	vector<unique_ptr<TrsOrder>> rule_removers;
	for( auto x : rulerem_specs ) {
		rule_removers.push_back(TrsOrder::of(x,sig,trs,mono,default_smt,Smt::INT));
	}

	// rule removal loop
	do {
		if( p.systems[0].empty() ) throw Ans::YES;
	} while( [&](){
		for( auto& proc : rule_removers ) {
			proc->print_name(cerr << "; trying ") << "... " << endl;
			auto const& rem = order_some_rule(*proc,p.systems[0]);
			if( rem.empty() ) {
				continue;
			}
			cout << "(remove-rule\n  ";
			proc->print( cout, p.sig ) << "\n ";
			for( size_t i : rem ) {
				cout << ' ' << i;
				p.systems[0].erase(i);
			}
			cout << ")" << endl;
			return true;
		}
		return false;
	}() );

	if( !use_dp ) throw Ans::MAYBE;

	Dps dps = make_dps(p.sig,p.systems[0]);
	cout << dps << endl;

	vector<unique_ptr<TrsOrder>> dp_removers;
	for( auto x : dprem_specs ) {
		dp_removers.push_back(TrsOrder::of(x,sig,trs,false,default_smt,Smt::INT));
	}
	// DP removal loop
	do {
		if( dps.empty() ) throw Ans::YES;
	} while( [&]{
		for( auto& proc : dp_removers ) {
			proc->print_name( cerr << "; trying " ) << "... " << endl;
			auto const& rem = order_some_dp(*proc,p.systems[0],dps);
			if( rem.empty() ) {
				continue;
			}
			cout << "(remove-dp\n  ";
			proc->print( cout, p.sig ) << "\n ";
			for( size_t i : rem ) {
				cout << ' ' << i;
				dps.erase(i);
			}
			cout << ')' << endl;
			return true;
		}
		return false;
	}() );
	throw Ans::MAYBE;
} catch( Ans a ) {
	switch( a ) {
	case Ans::YES:
		cout << "YES" << endl;
		exit(0);
	case Ans::NO:
		cout << "NO" << endl;
		exit(1);
	case Ans::MAYBE:
		cout << "MAYBE" << endl;
		exit(2);
	}
} catch( Error const& e ) {
	cerr << e << endl;
	exit(-1);
} catch( std::exception const& e ) {
	cerr << e.what() << endl;
	exit(-1);
}