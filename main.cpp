#include<map>
#include<fstream>
#include<fcntl.h>
#include"problem.hpp"
#include"termination.hpp"

using namespace std;


int main( int argc, char* argv[] ) {
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
		int i = 0;
		for( auto const& sys : p.systems ) {
			i++;
			cout << "TRS " << i << ":" << endl;
			int j = 0;
			for( auto const& rule : sys ) {
				j++;
				cout << '\t' << j << ": " << rule << endl;
			}
		}
		set<size_t> used;
		for( size_t i = 0; i < p.systems[0].size(); i++ ) {
			if( p.systems[0][i].weight != 0 ) {
				used.insert(i);
			}
		}
		auto solver = Smt::Z3(Smt::QF_NIA);
		auto proc = DerivedRuleRemover(p.sig,p.systems[0],used,Poly::Template::SUM,solver,Smt::INT);
		for(;;) {
			auto const& rem = proc.remove();
			if( rem.empty() ) {
				cout << "Failed." << endl;
				exit(0);
			}
			cout << "Removing";
			for( size_t i : rem ) {
				cout << ' ' << i+1;
			}
			cout << endl;
			for( auto [f,arity] : p.sig ) {
				cout << "[" << f << "] := " << Poly::eval_coeff(solver,proc.deriver(f)) << endl;
			}
			if( used.empty() ) {
				cout << "Terminating." << endl;
				exit(0);
			}
		}
	} catch( Trs::Reader::Error const& e ) {
		cerr << e << endl;
	} catch( Error const& e ) {
		cerr << e << endl;
	}
}