#include<iostream>
#include"smt.hpp"
#include"problem.hpp"

using namespace std;

int main( int argc, char const** argv ) try {
	Proc::test();
	Smt::test();
	Problem::test();
} catch( Exp::Error const& e ) {
	cerr << e << endl;
}