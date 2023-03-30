#include<iostream>
#include<cassert>
#include"smt.hpp"

using namespace std;

Smt::Exp Smt::Exp::operator&&( Exp const& other ) const {
	if( *this == FALSE ) {
		return *this;
	}
	if( *this == TRUE ) {
		return other;
	}
	if( other == FALSE ) {
		return other;
	}
	if( other == TRUE ) {
		return *this;
	}
	return _And{{*this,other}};
}

Smt::Exp Smt::Exp::operator||( Exp const& other ) const {
	if( *this == TRUE ) {
		return *this;
	}
	if( *this == FALSE ) {
		return other;
	}
	if( other == TRUE ) {
		return other;
	}
	if( other == FALSE ) {
		return *this;
	}
	return _Or{{*this,other}};
}

Smt::Exp Smt::Exp::operator!() const {
	if( *this == TRUE ) {
		return Exp(FALSE);
	}
	if( *this == FALSE ) {
		return Exp(TRUE);
	}
	return _Not{{*this}};
}

Smt::Exp Smt::Exp::operator+( Exp const& other ) const {
	if( *this == ZERO ) {
		return other;
	}
	if( other == ZERO ) {
		return *this;
	}
	return _Add{{*this,other}};
}

Smt::Exp Smt::Exp::operator*( Exp const& other ) const {
	if( *this == ZERO ) {
		return *this;
	}
	if( *this == ONE ) {
		return other;
	}
	if( other == ZERO ) {
		return other;
	}
	if( other == ONE ) {
		return *this;
	}
	return _Mul{{*this,other}};
}

ostream& operator<<( ostream& os, Smt::Exp const& e ) {
	if( auto sym = e.sym() ) {
		return os << *sym;
	} else if( auto args = e.conj() ) {
		return os << "(and " << args->first << ' ' << args->second << ')';
	} else if( auto args = e.disj() ) {
		return os << "(or " << args->first << ' ' << args->second << ')';
	} else if( auto arg = e.neg() ) {
		return os << "(not " << *arg << ')';
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
	cout << !(Exp("p") && Exp("q")) << endl;
	exit(0);
}
