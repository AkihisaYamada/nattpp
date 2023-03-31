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
	return {AND,*this,other};
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
	return {OR,*this,other};
}

Smt::Exp Smt::Exp::operator!() const {
	if( *this == TRUE ) {
		return FALSE;
	}
	if( *this == FALSE ) {
		return TRUE;
	}
	if( auto a = app() ) {
		if( a->first == NOT ) {
			return a->second[0];
		}
	}
	return {NOT,*this};
}

Smt::Exp Smt::Exp::eq( Exp const& other ) const {
	if( *this == other ) {
		return TRUE;
	}
	return {EQ,*this,other};
}

Smt::Exp Smt::ite( Exp const& x, Exp const& y, Exp const& z ) {
	if( x == TRUE ) {
		return y;
	}
	if( x == FALSE ) {
		return z;
	}
	return {ITE,x,y,z};
}

Smt::Exp Smt::Exp::operator>=( Exp const& other ) const {
	if( *this == other ) {
		return TRUE;
	}
	return {GE,*this,other};
}

Smt::Exp Smt::Exp::operator+( Exp const& other ) const {
	if( *this == ZERO ) {
		return other;
	}
	if( other == ZERO ) {
		return *this;
	}
	return {ADD,*this,other};
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
	return {MUL,*this,other};
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
		throw Error("#smt:no-response");
	}
	if( *ans == "sat" ) {
		_status = SAT;
		return *this;
	}
	if( *ans == "unsat" ) {
		_status = UNSAT;
		return *this;
	}
	throw Error({"#smt:invalid-response",*ans});
}

Smt::Exp Smt::Solver::declare_const( string_view const& name, string_view const& sort ) & {
	_proc.to << "(declare-const " << name << ' ' << sort << ')' << endl;
	return name;
}

Smt::Exp Smt::Solver::get_value( Exp const& e ) & {
	if( _status != SAT ) {
		throw Error("#smt:get_value");
	}
	_proc.to << "(get-value (" << e << "))" << endl;
	_reader.open();
	_reader.open();
	assert( _reader.read_sym() == e );
	Exp ret = _reader.read_exp();
	_reader.close();
	_reader.close();
	return ret;
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
	auto x = z3.declare_const("x","Int");
	z3.ass( x >= 5 );
	cout << z3.check_sat().result().is_sat() << endl;
	auto xv = z3.get_value(x);
	cout << x << " := " << xv << endl;
	exit(0);
} catch( Smt::Error const& e ) {
	cerr << e.message << endl;
	exit(-1);
}
