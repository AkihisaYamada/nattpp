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
	return Exp(_And{{*this,other}});
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
	return Exp(_Or{{*this,other}});
}

Smt::Exp Smt::Exp::operator!() const {
	if( *this == TRUE ) {
		return FALSE;
	}
	if( *this == FALSE ) {
		return TRUE;
	}
	if( auto arg = neg() ) {
		return *arg;
	}
	return Exp(_Not{{*this}});
}

Smt::Exp Smt::Exp::eq( Exp const& other ) const {
	if( *this == other ) {
		return TRUE;
	}
	return Exp(_Eq{{*this,other}});
}

Smt::Exp Smt::ite( Exp const& x, Exp const& y, Exp const& z ) {
	if( x == TRUE ) {
		return y;
	}
	if( x == FALSE ) {
		return z;
	}
	return Exp(Exp::_Ite{{x,y,z}});
}

Smt::Exp Smt::Exp::operator>=( Exp const& other ) const {
	if( *this == other ) {
		return TRUE;
	}
	return Exp(_Ge{{*this,other}});
}

Smt::Exp Smt::Exp::operator+( Exp const& other ) const {
	if( *this == ZERO ) {
		return other;
	}
	if( other == ZERO ) {
		return *this;
	}
	return Exp(_Add{{*this,other}});
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
	return Exp(_Mul{{*this,other}});
}

void Smt::Solver::set_logic( string_view const& x ) & {
	_proc.to << "(set-logic " << x << ')' << endl;
}

Smt::Solver& Smt::Solver::ass( Exp const& e ) & {
	if( _status != UNSAT ) {
		_proc.to << "(assert " << e << ')' << endl;
		_status = UNKNOWN;
	}
	return *this;
}

Smt::Solver& Smt::Solver::push() & {
	_proc.to << "(push)" << endl;
	_status = UNKNOWN;
	return *this;
}

Smt::Solver& Smt::Solver::pop() & {
	_proc.to << "(pop)" << endl;
	_status = UNKNOWN;
	return *this;
}

Smt::Solver& Smt::Solver::check_sat() & {
	if( _status == UNKNOWN ) {
		_proc.to << "(check-sat)" << endl;
		_status = SOLVING;
	}
	return *this;
}
Smt::Solver& Smt::Solver::result() & {
	assert( _status == SOLVING );
	auto ans = _reader.reads_exp();
	if( !ans ) {
		throw Error("#no-response");
	}
	if( *ans == "sat" ) {
		_status = SAT;
		return *this;
	}
	if( *ans == "unsat" ) {
		_status = UNSAT;
		return *this;
	}
	throw Error({"#invalid-smt-response",*ans});
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
	} else if( auto args = e.ite() ) {
		return os << "(ite " << get<0>(*args) << ' ' << get<1>(*args) << ' ' << get<2>(*args) << ')';
	} else if( auto args = e.eq() ) {
		return os << "(= " << args->first << ' ' << args->second << ')';
	} else if( auto args = e.ge() ) {
		return os << "(>= " << args->first << ' ' << args->second << ')';
	} else if( auto args = e.gt() ) {
		return os << "(> " << args->first << ' ' << args->second << ')';
	} else if( auto args = e.add() ) {
		return os << "(+ " << args->first << ' ' << args->second << ')';
	} else if( auto args = e.mul() ) {
		return os << "(* " << args->first << ' ' << args->second << ')';
	} else {
		assert(false);
	}
}

int Smt::test() try {
	cout << "this is Smt::test()." << endl;
	cout << Exp("1") + "x" << endl;
	cout << !!(Exp("0") + "x") << endl;
	cout << ite( "p", Exp("3") * "x" * "y", Smt::ZERO ) << endl;
	cout << !(Exp("x").eq("y") && Exp("y") >= "3") << endl;
	auto z3 = Smt::Z3();
	Proc ps = Proc("ps",{"ps"});
	for( char c; (c=ps.from.get()) != EOF; cout.put(c) ) ;
	z3.set_logic("QF_LIA");
	z3.ass("false");
	cout << z3.check_sat().result().is_unsat() << endl;
	exit(0);
} catch( Smt::Error const& e ) {
	cerr << e.message << endl;
	exit(-1);
}
