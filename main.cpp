#include<map>
#include<fstream>
#include<fcntl.h>
#include"problem.hpp"

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
		Problem p(*pis);
		p.write_systems(cout);
	} catch( Term::Reader::Error const& e ) {
		cerr << e.msg << endl;
	} catch( Exp::Error const& e ) {
		cerr << e.msg << endl;
	}
}