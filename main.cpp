#include<map>
#include<fstream>
#include<fcntl.h>
#include"problem.hpp"
#include"termination.hpp"

using namespace std;


int main( int argc, char* argv[] ) {
	try {
		istream* pis = nullptr;
		ostream* ptee = nullptr;
		bool exit_on_error = false;
		for( int i = 1; i < argc; i++ ) {
			if( argv[i][0] == '-' ) {
				string_view arg = argv[i];
				if( arg == "-tee" ) {
					if( ptee != nullptr ) throw Error("#duplicate-tee");
					i++;
					if( i == argc ) throw Error("#missing-tee-file");
					ptee = new ofstream(argv[i]);
					continue;
				}
				throw Error("#unknown-option",argv[i]);
			} else {
				if( pis != nullptr ) throw Error("#too-many-arguments",argv[i]);
				pis = new ifstream(argv[i]);
				if( pis->fail() ) throw Error("#open-failed",argv[i]);
				exit_on_error = true;
			}
		}
		if( pis == nullptr ) pis = &cin;
		auto p = Problem(*pis);
		int i = 0;
		for( auto const& sys : p.systems ) {
			i++;
			cout << "TRS " << i << ":" << endl;
			sys.print(cout,1,"  ");
		}
		set<size_t> used;
		for( size_t i = 0; i < p.systems[0].size(); i++ ) {
			if( p.systems[0][i].weight != 0 ) {
				used.insert(i);
			}
		}
		auto solver = Smt::Z3(Smt::QF_NIA, ptee ? Opt<ostream&>{*ptee} : Opt<ostream&>{} );
		auto proc = DerivedRuleRemover<Poly>(p.sig,p.systems[0],used,Poly::Template::SUM,solver,Smt::INT);
		for(;;) {
			auto const& rem = proc.remove();
			if( rem.empty() ) {
				cout << "Failed." << endl;
				exit(0);
			}
			cout << "(remove-rule";
			for( size_t i : rem ) {
				cout << ' ' << i+1;
			}
			cout << "\n  :interpretation (";
			for( auto [f,arity] : p.sig ) {
				cout << "\n    (" << f << ' ' << Poly::eval_coeff(solver,proc.deriver(f)) << ')';
			}
			cout << "))" << endl;
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