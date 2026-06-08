#include<map>
#include<fstream>
#include<fcntl.h>
#include"poly.hpp"
#include"problem.hpp"
#include"termord.hpp"
#include"deprem.hpp"

using namespace std;

int main( int argc, char* argv[] ) try {
	istream* pis = nullptr;
	ostream* ptee = nullptr;
	ostream* pprf = nullptr;
	bool exit_on_error = false;
	Opt<Exp> solverexp;
	for( int i = 1; i < argc; i++ ) {
		if( argv[i][0] == '-' ) {
			string_view opt = argv[i];
			i++;
			if( i == argc ) throw Error("#missing-arg",opt);
			if( opt == "-proof" ) {
				if( pprf != nullptr ) throw Error("#duplicate-option",opt);
				pprf = new ofstream(argv[i]);
			} else if( opt == "-solver" ) {
				solverexp = Exp::of(argv[i]);
			} else {
				throw Error("#unknown-option",argv[i]);
			}
		} else {
			if( pis != nullptr ) throw Error("#too-many-arguments",argv[i]);
			pis = new ifstream(argv[i]);
			if( pis->fail() ) throw Error("#open-failed",argv[i]);
			exit_on_error = true;
		}
	}
	if( pis == nullptr ) pis = &cin;
	auto mksolver = [&]()->Smt::Solver{
		if( solverexp ) {
			return Smt::Solver::of(*solverexp);
		} else {
			return Smt::Z3(Smt::QF_LIA);
		}
	};
	auto p = Problem(*pis);
	cout << p << endl;

	vector<unique_ptr<TrsOrder>> rule_removers;
	rule_removers.push_back(
		make_unique<DerivedTrsOrder<Poly>>(
			p.sig, p.systems[0], Poly::Template::MONO_SUM, mksolver(), Smt::INT
		)
	);

	// rule removal loop
	do {
		if( p.systems[0].empty() ) throw true;
	} while( [&](){
		for( auto& proc : rule_removers ) {
			proc->print_name(cerr << "; trying ") << "... " << endl;
			auto const& rem = order_some_rule(*proc,p.systems[0]);
			if( rem.empty() ) {
				continue;
			}
			cout << "(remove-rule";
			for( size_t i : rem ) {
				cout << ' ' << i;
				p.systems[0].erase(i);
			}
			proc->print( cout << "\n  :order ", p.sig ) << ")" << endl;
			return true;
		}
		return false;
	}() );

	Dps dps = make_dps(p.sig,p.systems[0]);
	cout << dps << endl;

	vector<unique_ptr<TrsOrder>> dp_removers;
	dp_removers.push_back(
		make_unique<DerivedTrsPosOrder<Poly>>
			(p.sig, p.systems[0], Poly::Template::SUM, mksolver(), Smt::INT )
	);
	dp_removers.push_back(
		make_unique<PathOrder>(p.sig,p.systems[0],make_unique<TrivOrder>(mksolver()))
	);
	// DP removal loop
	do {
		if( dps.empty() ) throw true;
	} while( [&]{
		for( auto& proc : dp_removers ) {
			proc->print_name( cerr << "; trying " ) << "... " << endl;
			auto const& rem = order_some_dp(*proc,p.systems[0],dps);
			if( rem.empty() ) {
				continue;
			}
			cout << "(remove-dp";
			for( size_t i : rem ) {
				cout << ' ' << i;
				dps.erase(i);
			}
			proc->print( cout << "\n  :order ", p.sig ) << ")" << endl;
			return true;
		}
		return false;
	}() );
	cout << "failed" << endl;
	exit(-1);
} catch( bool b ) {
	if( b ) {
		cout << "terminating" << endl;
		exit(0);
	}
	cout << "nonterminating" << endl;
	exit(1);
} catch( Error const& e ) {
	cerr << e << endl;
	exit(-1);
}