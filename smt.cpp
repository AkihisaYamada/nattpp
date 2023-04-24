#include<iostream>
#include<cassert>
#include"smt.hpp"

using namespace std;

Smt::Logic const Smt::QF_LIA = "QF_LIA";
Smt::Logic const Smt::QF_LRA = "QF_LRA";
Smt::Logic const Smt::LIA = "LIA";
Smt::Logic const Smt::LRA = "LRA";
Smt::Logic const Smt::QF_IA = "QF_IA";
Smt::Logic const Smt::QF_RA = "QF_RA";
Smt::Logic const Smt::IA = "IA";
Smt::Logic const Smt::RA = "RA";
Smt::BaseSort const Smt::BOOL = "Bool";
Smt::BaseSort const Smt::INT = "Int";
Smt::BaseSort const Smt::REAL = "Real";

Smt::Const const Smt::TRUE = "true";
Smt::Const const Smt::FALSE = "false";
Smt::Const const Smt::ZERO = "0";
Smt::Const const Smt::ONE = "1";

ostream& operator<<( ostream& os, Smt::PreExp const& e ) {
	if( auto app = e.app() ) {
		auto const& [fun,args] = *app;
		if( args.empty() ) {
			return os << fun;
		}
		os << '(' << fun;
		for( auto const& arg : args ) {
			os << ' ' << arg;
		}
		return os << ')';
	}
	if( auto let = e.let() ) {
		auto const& [val,sort,body] = *let;
		return os << "(let " << val << ' ' << sort << " ...)";
	}
	if( auto lazy = e.lazy() ) {
		return os << "...";
	}
	assert(false);
}

Smt::PostExp& Smt::PostExp::disj_eq( Smt::PostExp const& arg ) & {
	if( _exp == TRUE ) {
		return *this;
	}
	if( _exp == FALSE ) {
		_exp = arg._exp;
		return *this;
	}
	if( _exp.fun() == OR ) {
		_exp.args().push_back(arg._exp);
		return *this;
	}
	PostExp ret = Exp{OR,_exp,arg._exp};
	return *this = ret;
}

Smt::PostExp& Smt::PostExp::conj_eq( Smt::PostExp const& arg ) & {
	if( _exp == FALSE ) {
		return *this;
	}
	if( _exp == TRUE ) {
		_exp = arg._exp;
		return *this;
	}
	if( _exp.fun() == AND ) {
		_exp.args().push_back(arg._exp);
		return *this;
	}
	PostExp ret = Exp{AND,_exp,arg._exp};
	return *this = ret;
}

Algebra::Intp<Smt::PreExp> const Smt::ALGEBRA = []( string_view const& fun, vector<Smt::PreExp>&& args ){
	return Smt::PreExp(fun,std::move(args));
};

Smt::Solver::Solver( Proc& proc, Logic const& logic ) : _status(UNKNOWN), _proc(proc), _reader(proc.from), _var_count(0) {
	_proc.to << "(set-logic " << logic.str << ')' << endl;
}

Smt::Solver& Smt::Solver::ass( PostExp const& e ) & {
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

Smt::Const Smt::Solver::declare_const( string_view const& name, BaseSort const& sort ) & {
	_proc.to << "(declare-const " << name << ' ' << sort.name << ')' << endl;
	return name;
}

Smt::Const Smt::Solver::define_fun(
	std::string_view const& name,
	std::initializer_list<std::pair<std::string_view,std::string_view>> const& params,
	BaseSort const& sort,
	PostExp const& body
) & {
	_proc.to << "(define-fun " << name << " (";
	for( auto [var,psort] : params ) {
		_proc.to << '(' << var << ' ' << psort << ") ";
	}
	_proc.to << ") " << sort.name << ' ' << body << ')' << endl;
	return name;
}

Smt::PostExp Smt::Solver::get_value( PostExp const& e ) & {
	if( _status != SAT ) {
		throw Error("#smt:get_value");
	}
	_proc.to << "(get-value (" << e << "))" << endl;
	_reader.open();
	_reader.open();
	assert( _reader.read_sym() == e._exp );
	PostExp ret = _reader.read_exp();
	_reader.close();
	_reader.close();
	return ret;
}

std::string Smt::Solver::_make_fresh() & {
	return string("_") + to_string(_var_count++);
}

Smt::PostExp Smt::Car( PostExp const& arg ) {
	if( arg._exp.fun() != CONS ) {
		throw Error{"#car-on",arg._exp};
	}
	auto const& args = arg._exp.args();
	assert( args.size() == 2 );
	return args[0];
}
Smt::PostExp Smt::Cdr( PostExp const& arg ) {
	if( arg._exp.fun() != CONS ) {
		throw Error{"#cdr-on",arg._exp};
	}
	auto const& args = arg._exp.args();
	assert( args.size() == 2 );
	return args[1];
}

Smt::PostExp Smt::Solver::expand( PreExp const& p ) {
	if( auto app = p.app() ) {
		auto const& [fun,args] = *app;
		auto ret = Exp(fun);
		auto& eargs = ret.args();
		if( fun == AND ) {
			for( auto const& arg : args ) {
				auto const& earg = expand(arg)._exp;
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
		} else if( fun == OR ) {
			for( auto const& arg : args ) {
				auto const& earg = expand(arg)._exp;
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
		} else if( fun == NOT ) {
			assert( args.size() == 1 );
			auto const& earg = expand(args[0])._exp;
			if( earg == TRUE ) {
				return FALSE;
			}
			if( earg == FALSE ) {
				return TRUE;
			}
			if( earg.fun() == NOT ) {
				return earg.args()[0];
			}
			eargs.push_back(earg);
		} else if( fun == ITE ) {
			assert( args.size() == 3 );
			auto const& i = expand(args[0])._exp;
			if( i == TRUE ) {
				return expand(args[1]);
			}
			if( i == FALSE ) {
				return expand(args[2]);
			}
			eargs.push_back(i);
			eargs.push_back(expand(args[1])._exp);
			eargs.push_back(expand(args[2])._exp);
		} else if( fun == ADD ) {
			for( auto const& arg : args ) {
				auto const& earg = expand(arg)._exp;
				if( earg != ZERO ) {
					eargs.push_back(earg);
				}
			}
			switch( eargs.size() ) {
				case 0: return ZERO;
				case 1: return eargs[0];
			}
		} else if( fun == MUL ) {
			for( auto const& arg : args ) {
				auto const& earg = expand(arg)._exp;
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
		} else if( fun == CAR ) {
			assert( args.size() == 1 );
			return Car(expand(args[0]));
		} else if( fun == CDR ) {
			assert( args.size() == 1 );
			return Cdr(expand(args[0]));
		} else {
			for( auto const& arg : args ) {
				eargs.push_back(expand(arg)._exp);
			}
		}
		return ret;
	}
	if( auto let = p.let() ) {
		auto const& [val,sort,body] = *let;
		return expand(body(_expand_let(sort._exp,expand(val))));
	}
	if( auto lazy = p.lazy() ) {
		return expand((*lazy)());
	}
	assert(false);
};
Smt::PreExp Smt::Solver::_expand_let( Sort const& sort, PostExp const& val ) {
	if( auto const& base = sort.base() ) {
		if( val._exp.args().empty() ) {
			return val._exp.fun();
		}
		auto var = _make_fresh();
		return define_fun(var,{},*base,val);
	}
	auto const& sargs = sort._exp.args();
	assert( val._exp.fun() == CONS );
	auto const& vargs = val._exp.args();
	auto const& v1 = _expand_let(sargs[0],vargs[0]);
	auto const& v2 = _expand_let(sargs[1],vargs[1]);
	return (v1,v2);
};

int Smt::test() try {
	cout << "this is Smt::test()." << endl;
	cout << PreExp(1) + "x" << endl;
	cout << !!(PreExp(0) + []{ return "x"; }) << endl;
	cout << (If("p") ^ PreExp(3) * "x" * "y" ^ ZERO) << endl;
	cout << !(PreExp("x").eq("y") && PreExp("y").ge(3)) << endl;
	auto z3 = Z3(QF_LIA,cout);
	auto x = z3.declare_const("x","Int");
	auto five = z3.define_fun("five",{},Smt::INT,5);
	z3.ass( x.gt(five + 4) );
	cout << z3.check_sat().result().is_sat() << endl;
	auto xv = z3.get_value(x);
	cout << x << " := " << xv << endl;

	cout << z3.expand( FALSE && []{ return PreExp("BUG"); } ) << endl;

	cout << z3.expand( TRUE || []{ return PreExp("BUG"); } ) << endl;

	cout << z3.expand( If( TRUE ) ^ []{ return PreExp("ok"); } ^ []{ return PreExp("BUG"); } ) << endl;

	cout << z3.expand( If( FALSE ) ^ []{ return PreExp("BUG"); } ^ []{ return PreExp("ok"); } ) << endl;

	cout << z3.expand( If( "cond" ) ^ []{ return PreExp("then"); } ^ []{ return PreExp("else"); } ) << endl;

	z3.ass(
		Let(INT) ^ PreExp(x) + "five" ^ []( PreExp const& x5 ){ return (x5 + x5).ge("20"); }
	);

	cout << z3.check_sat().result().is_sat() << endl;

	auto car_xy = Car((x,"y"));
	cout << car_xy << endl;
	cout << z3.expand(car_xy) << endl;

	auto cons_let = Let((BOOL,INT)) ^ (TRUE,x) ^ []( PreExp const& pair ) { return Cdr(pair); };
	cout << cons_let << endl;
	cout << z3.expand(cons_let) << endl;

	return 0;
} catch( ::Exp::Error const& e ) {
	cerr << e << endl;
	return -1;
}
