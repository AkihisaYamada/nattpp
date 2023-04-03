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

Smt::Exp Smt::Exp::ge( Exp const& other ) const {
	if( *this == other ) {
		return TRUE;
	}
	return {GE,*this,other};
}

Smt::Exp Smt::Exp::gt( Exp const& other ) const {
	if( *this == other ) {
		return FALSE;
	}
	return {GT,*this,other};
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
	throw Error{"#smt:invalid-response",*ans};
}

Smt::Exp Smt::Solver::declare_const( string_view const& name, string_view const& sort ) & {
	_proc.to << "(declare-const " << name << ' ' << sort << ')' << endl;
	return name;
}

Smt::Exp Smt::Solver::define_fun(
	std::string_view const& name,
	std::initializer_list<std::pair<std::string_view,std::string_view>> const& params,
	std::string_view const& sort,
	Exp const& body
) & {
	_proc.to << "(define-fun " << name << " (";
	for( auto [var,psort] : params ) {
		_proc.to << '(' << var << ' ' << psort << ") ";
	}
	_proc.to << ") " << sort << ' ' << body << ')' << endl;
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

std::string Smt::Solver::_make_fresh() & {
	return string("_") + to_string(_var_count++);
}

Smt::Exp Smt::Solver::expand( PreExp const& p ) {
	if( auto sym = p.sym() ) {
		return *sym;
	}
	if( auto app = p.app() ) {
		auto it = app->begin(), end = app->end();
		if( it == end ) {
			return Exp();
		}
		auto fun = expand(*it);
		it++;
		auto args = std::vector<::Exp>();
		while( it != end ) {
			args.push_back(expand(*it));
			it++;
		}
		return Exp(fun,std::move(args));
	}
	if( auto let = p.let() ) {
		auto [val,sort,body] = *let;
		auto eval = expand(val);
		auto var = _make_fresh();
		define_fun(var,{},sort,eval);
		return expand(body(PreExp(var)));
	}
	if( auto lazy = p.lazy() ) {
		auto [op,x,y] = *lazy;
		auto ex = expand(x);
		if( op == AND ) {
			if( ex == FALSE ) {
				return ex;
			}
			auto ey = expand(y());
			if( ex == TRUE ) {
				return ey;
			}
			return {AND,ex,ey};
		}
		if( op == OR ) {
			if( ex == TRUE ) {
				return ex;
			}
			auto ey = expand(y());
			if( ex == FALSE ) {
				return ey;
			}
			return {OR,ex,ey};
		}
	}
	if( auto lazy3 = p.lazy3() ) {
		auto [op,x,y,z] = *lazy3;
		if( op == ITE ) {
			auto ex = expand(x);
			if( ex == TRUE ) {
				return expand(y());
			}
			if( ex == FALSE ) {
				return expand(z());
			}
			auto ey = expand(y()), ez = expand(z());
			return {ITE,ex,ey,ez};
		}
	}
	assert(false);
};


int Smt::test() try {
	cout << "this is Smt::test()." << endl;
	cout << Exp(1) + "x" << endl;
	cout << !!(Exp(0) + "x") << endl;
	cout << ite( "p", Exp(3) * "x" * "y", Smt::ZERO ) << endl;
	cout << !(Exp("x").eq("y") && Exp("y").ge(3)) << endl;
	auto z3 = Z3();
	z3.set_logic("QF_LIA");
	auto x = z3.declare_const("x","Int");
	auto five = z3.define_fun("five",{},"Int",5);
	z3.ass( x.gt(five + 4) );
	cout << z3.check_sat().result().is_sat() << endl;
	auto xv = z3.get_value(x);
	cout << x << " := " << xv << endl;

	auto lazy_true = PreExp(TRUE) || [&](){ return PreExp("BUG"); };
	cout << z3.expand( lazy_true ) << endl;

	cout << z3.expand( ite(PreExp(TRUE),[&](){ return PreExp("ok"); },[&](){ return PreExp("BUG"); }) ) << endl;

	cout << z3.expand( ite(FALSE,[&](){ return PreExp("BUG"); },[&](){ return PreExp("ok"); }) ) << endl;

	cout << z3.expand( ite(PreExp("if"),[&](){ return PreExp("then"); },[&](){ return PreExp("else"); }) ) << endl;

	auto foo = PreExp(LET,PreExp("x")+"five","Int",[&](PreExp const& x5){ return (x5 + x5).ge("20") ; });
	z3.ass(foo);

	cout << z3.check_sat().result().is_sat() << endl;

	return 0;
} catch( Exp::Error const& e ) {
	cerr << e << endl;
	return -1;
}
