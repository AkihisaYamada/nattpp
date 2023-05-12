#include<map>
#include<fstream>
#include<fcntl.h>
#include"problem.hpp"
#include"termination.hpp"

using namespace std;


int main( int argc, char** argv ) {
	try {
		istream* pis;
		bool exit_on_error = false;
		if( argc == 1 ) {
			pis = &cin;
		} else {
			pis = new fstream(argv[1]);
			exit_on_error = true;
		}
		auto p = Problem(*pis);
		for( int i = 0; i < p.systems.size(); i++ ) {
			cout << "TRS " << i+1 << ":" << endl << p.systems[i];
		}
		set<size_t> used;
		for( size_t i = 0; i < p.systems[0].size(); i++ ) {
			used.insert(i);
		}
		auto z3 = Smt::Z3(Smt::QF_LIA,cout);
		auto proc = DerivedRuleRemover(p.sig,p.systems[0],used,Template::SUM,z3);
		for(;;) {
			auto const& rem = proc.remove();
			if( rem.empty() ) {
				cout << "Failed." << endl;
				exit(0);
			}
			cout << "Removing";
			for( size_t i : rem ) {
				cout << ' ' << i;
			}
			cout << endl;
			if( used.empty() ) {
				cout << "Terminating." << endl;
				exit(0);
			}
		}
	} catch( Trs::Reader::Error const& e ) {
		cerr << e << endl;
	} catch( Exp::Error const& e ) {
		cerr << e << endl;
	}
}