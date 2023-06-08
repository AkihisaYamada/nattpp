#include<iostream>
#include"smt.hpp"
#include"problem.hpp"
#include"poly.hpp"

using namespace std;

int main( int argc, char const** argv ) try {
	cout << "=== Opt test ===" << endl;
	Opt<int> oi = {};
	if( oi ) {
		throw Error("Failed");
	}
	Proc::test();
	Smt::test();
	Problem::test();
	Poly::test();
	cout << "=== Test Done ===" << endl;
} catch( Error const& e ) {
	cerr << e << endl;
}