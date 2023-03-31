#include<map>
#include<fstream>
#include<fcntl.h>
#include"parser.hpp"

using namespace std;


int main( int argc, char** argv ) {
	try {
		Parser obj;
		istream* pis;
		bool exit_on_error = false;
		if( argc == 1 ) {
			pis = &cin;
		} else {
			pis = new fstream(argv[1]);
			exit_on_error = true;
		}
		obj.parse(*pis);
		obj.write_systems(cout);
	} catch( Term::Reader::Error const& e ) {
		cerr << e.msg << endl;
	} catch( Exp::Error const& e ) {
		cerr << e.msg << endl;
	}
}