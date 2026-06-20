#include<iostream>
#include"smt.hpp"
#include"problem.hpp"
#include"poly.hpp"
#include"termord.hpp"
#include"graph.hpp"

using namespace std;

int main( int argc, char const** argv ) try {
	Proc::test();
	Smt::test();
	Problem::test();
	Poly::test();
	Template::test();
	TermOrder::test();
	Graph::test();
	cout << "=== Test Done ===" << endl;
} catch( Error const& e ) {
	cerr << e << endl;
}