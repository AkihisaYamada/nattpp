#include<iostream>
#include<cassert>
#include"smt.hpp"

using namespace std;

ostream& operator<<( ostream& os, Smt::Exp const& e ) {
	if( auto sym = e.sym() ) {
		return os << *sym;
	}
	if( auto app = e.app() ) {
		os << '(' << app->first;
		for( auto const& arg : app->second ) {
			os << ' ' << arg;
		}
		return os << ')';
	}
	if( auto let = e.let() ) {
		auto const& [val,sort,body] = *let;
		return os << '(' << val << ' ' << sort << " ...)";
	}
	if( auto lazy = e.lazy() ) {
		return os << "...";
	}
	assert(false);
}

void Smt::Solver::set_logic( string_view const& x ) & {
	_proc.to << "(set-logic " << x << ')' << endl;
}

Smt::Solver& Smt::Solver::ass( ::Exp const& e ) & {
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

void Smt::Solver::declare_const( string_view const& name, string_view const& sort ) & {
	_proc.to << "(declare-const " << name << ' ' << sort << ')' << endl;
}

Smt::Exp Smt::Solver::define_fun(
	std::string_view const& name,
	std::initializer_list<std::pair<std::string_view,std::string_view>> const& params,
	std::string_view const& sort,
	Exp const& body
) & {
	auto const& ebody = expand(body);
	_proc.to << "(define-fun " << name << " (";
	for( auto [var,psort] : params ) {
		_proc.to << '(' << var << ' ' << psort << ") ";
	}
	_proc.to << ") " << sort << ' ' << ebody << ')' << endl;
	return name;
}

::Exp Smt::Solver::get_value( ::Exp const& e ) & {
	if( _status != SAT ) {
		throw Error("#smt:get_value");
	}
	_proc.to << "(get-value (" << e << "))" << endl;
	_reader.open();
	_reader.open();
	assert( _reader.read_sym() == e );
	::Exp ret = _reader.read_exp();
	_reader.close();
	_reader.close();
	return ret;
}

std::string Smt::Solver::_make_fresh() & {
	return string("_") + to_string(_var_count++);
}

::Exp Smt::Solver::expand( Exp const& p ) {
	if( auto sym = p.sym() ) {
		return *sym;
	}
	if( auto app = p.app() ) {
		auto const& [fun,args] = *app;
		auto const& efun = expand(fun);
		auto eargs = std::vector<::Exp>();
		if( efun == AND ) {
			for( auto const& arg : args ) {
				auto const& earg = expand(arg);
				if( earg == FALSE ) {
					return FALSE;
				}
				if( earg != TRUE ) {
					eargs.push_back(earg);
				}
			}
			switch( eargs.size() ) {
				case 0: return TRUE;
				case 1: return eargs[0];
			}
		} else if( efun == OR ) {
			for( auto const& arg : args ) {
				auto const& earg = expand(arg);
				if( earg == TRUE ) {
					return TRUE;
				}
				if( earg != FALSE ) {
					eargs.push_back(earg);
				}
			}
			switch( eargs.size() ) {
				case 0: return FALSE;
				case 1: return eargs[0];
			}
		} else if( efun == NOT ) {
			assert( args.size() == 1 );
			auto const& earg = expand(args[0]);
			if( earg == TRUE ) {
				return FALSE;
			}
			if( earg == FALSE ) {
				return TRUE;
			}
			if( auto app = earg.app() ) {
				if( app->first == NOT ) {
					return app->second[0];
				}
			}
			eargs.push_back(earg);
		} else if( efun == ITE ) {
			assert( args.size() == 3 );
			auto const& i = expand(args[0]);
			if( i == TRUE ) {
				return expand(args[1]);
			}
			if( i == FALSE ) {
				return expand(args[2]);
			}
			eargs.push_back(i);
			eargs.push_back(expand(args[1]));
			eargs.push_back(expand(args[2]));
		} else if( efun == ADD ) {
			for( auto const& arg : args ) {
				auto const& earg = expand(arg);
				if( earg != ZERO ) {
					eargs.push_back(earg);
				}
			}
			switch( eargs.size() ) {
				case 0: return ZERO;
				case 1: return eargs[0];
			}
		} else if( efun == MUL ) {
			for( auto const& arg : args ) {
				auto const& earg = expand(arg);
				if( earg == ZERO ) {
					return ZERO;
				}
				if( earg != ONE ) {
					eargs.push_back(earg);
				}
			}
			switch( eargs.size() ) {
				case 0: return ONE;
				case 1: return eargs[0];
			}
		} else {
			for( auto const& arg : args ) {
				eargs.push_back(expand(arg));
			}
		}
		return ::Exp(efun,std::move(eargs));
	}
	if( auto let = p.let() ) {
		auto [val,sort,body] = *let;
		auto var = _make_fresh();
		define_fun(var,{},sort,val);
		return expand(body(Exp(var)));
	}
	if( auto lazy = p.lazy() ) {
		return expand((*lazy)());
	}
	assert(false);
};


int Smt::test() try {
	cout << "this is Smt::test()." << endl;
	cout << Exp(1) + "x" << endl;
	cout << !!(Exp(0) + []()->Exp{ return "x"; }) << endl;
	cout << (If("p") ^ Exp(3) * "x" * "y" ^ ZERO) << endl;
	cout << !(Exp("x").eq("y") && Exp("y").ge(3)) << endl;
	auto z3 = Z3(cout);
	z3.set_logic("QF_LIA");
	z3.declare_const("x","Int");
	auto five = z3.define_fun("five",{},"Int",5);
	z3.ass( Smt::Exp("x").gt(five + 4) );
	cout << z3.check_sat().result().is_sat() << endl;
	auto xv = z3.get_value("x");
	cout << "x" << " := " << xv << endl;

	cout << z3.expand( Exp(FALSE) && []{ return Exp("BUG"); } ) << endl;

	cout << z3.expand( Exp(TRUE) || []{ return Exp("BUG"); } ) << endl;

	cout << z3.expand( If( TRUE ) ^ []{ return Exp("ok"); } ^ []{ return Exp("BUG"); } ) << endl;

	cout << z3.expand( If( FALSE ) ^ []{ return Exp("BUG"); } ^ []{ return Exp("ok"); } ) << endl;

	cout << z3.expand( If( "cond" ) ^ []{ return Exp("then"); } ^ []{ return Exp("else"); } ) << endl;

	z3.ass(
		Let( Exp("x") + "five", "Int" ) ^ [](Exp const& x5){
			return (x5 + x5).ge("20");
		}
	);

	cout << z3.check_sat().result().is_sat() << endl;

	return 0;
} catch( ::Exp::Error const& e ) {
	cerr << e << endl;
	return -1;
}
