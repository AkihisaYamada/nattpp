#include<iostream>
#include"smt.hpp"
#include"problem.hpp"
#include"poly.hpp"
#include"termord.hpp"

using namespace std;

int main( int argc, char const** argv ) try {
	Algebra::test();
	Proc::test();
	Smt::test();
	Problem::test();
	Poly::test();
	TermOrder::test();
	cout << "=== Test Done ===" << endl;
} catch( Error const& e ) {
	cerr << e << endl;
}