#include<iostream>
#include<cassert>
#include"smt.hpp"

using namespace std;

Smt::Exp Smt::Exp::operator+( Exp const& other ) const {
	if( *this == "0" ) {
		return other;
	}
	if( other == "0" ) {
		return *this;
	}
	return _Add(*this,other);
}

Smt::Exp Smt::Exp::operator*( Exp const& other ) const {
	if( *this == "1" ) {
		return other;
	}
	if( other == "1" ) {
		return *this;
	}
	return _Mul(*this,other);
}

ostream& operator<<( ostream& os, Smt::Exp const& e ) {
	if( auto sym = e.sym() ) {
		return os << *sym;
	} else if( auto args = e.add() ) {
		return os << "(+ " << args->first << ' ' << args->second << ')';
	} else if( auto args = e.mul() ) {
		return os << "(* " << args->first << ' ' << args->second << ')';
	} else {
		assert(false);
	}
}

int Smt::test() {
	cout << "this is Smt::test()." << endl;
	cout << Exp("1") + Exp("x") << endl;
	cout << Exp("0") + Exp("x") << endl;
	cout << Exp("3") * Exp("x") * Exp("y") << endl;
	exit(0);
}
