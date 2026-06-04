#include<map>
#include<fstream>
#include<fcntl.h>
#include"problem.hpp"
#include"termination.hpp"
#include"dp.hpp"

using namespace std;


int main( int argc, char* argv[] ) {
	try {
		istream* pis = nullptr;
		ostream* ptee = nullptr;
		ostream* pprf = nullptr;
		bool exit_on_error = false;
		for( int i = 1; i < argc; i++ ) {
			if( argv[i][0] == '-' ) {
				string_view arg = argv[i];
				if( arg == "-tee" ) {
					if( ptee != nullptr ) throw Error("#duplicate-option","-tee");
					i++;
					if( i == argc ) throw Error("#missing-arg","-tee");
					ptee = new ofstream(argv[i]);
					continue;
				}
				if( arg == "-proof" ) {
					if( pprf != nullptr ) throw Error("#duplicate-option","-proof");
					i++;
					if( i == argc ) throw Error("#missing-arg","-proof");
					pprf = new ofstream(argv[i]);
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
		cout << p << endl;
		set<size_t> actives;
		for( size_t i = 0; i < p.systems[0].size(); i++ ) {
			if( p.systems[0][i].weight != 0 ) {
				actives.insert(i);
			}
		}
		auto solver = Smt::Z3(Smt::QF_NIA, ptee ? Opt<ostream&>{*ptee} : Opt<ostream&>{} );
		vector<RuleRemover> rule_removers;
		rule_removers.emplace_back(
			DerivedTermOrder<Poly>(p.sig,Poly::Template::MONO_SUM,solver,Smt::INT),
			p.systems[0],actives,solver
		);
		auto rule_remove = [&](){
			for( auto& proc : rule_removers ) {
				proc.print_name(cerr << "; trying ") << "... " << endl;
				auto const& rem = proc.remove();
				if( rem.empty() ) {
					continue;
				}
				cout << "(remove-rule";
				for( size_t i : rem ) {
					cout << ' ' << i+1;
				}
				proc.print( cout << "\n  :order ", p.sig ) << ")" << endl;
				return true;
			}
			return false;
		};
		while( rule_remove() ) {
			if( actives.empty() ) {
				cout << "terminating" << endl;
				exit(0);
			}
		}
		Dp::Rules drules;
		for( auto i : actives ) {
			drules.emplace_back(p.systems[0][i],p.sig);
		}
		cout << drules << endl;
		cout << "failed" << endl;
		exit(-1);
	} catch( Trs::Reader::Error const& e ) {
		cerr << e << endl;
	} catch( Error const& e ) {
		cerr << e << endl;
	}
}