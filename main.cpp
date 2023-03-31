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
		for( int i = 0; i < p.systems.size(); i++ ) {
			cout << "TRS " << i+1 << ":" << endl << p.systems[i];
		}
	} catch( TRS::Reader::Error const& e ) {
		cerr << e << endl;
	} catch( Exp::Error const& e ) {
		cerr << e << endl;
	}
}