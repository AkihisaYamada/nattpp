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

Smt::PostExp const Smt::TRUE = "true";
Smt::PostExp const Smt::FALSE = "false";
Smt::PostExp const Smt::ZERO = "0";
Smt::PostExp const Smt::ONE = "1";

ostream& operator<<( ostream& os, Smt::PreExp const& e ) {
	if( auto post = e.post() ) {
		return os << *post;
	}
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
	if( *this == TRUE ) {
		return *this;
	}
	if( *this == FALSE ) {
		Exp::operator=(arg);
		return *this;
	}
	if( fun() == OR ) {
		args().push_back(arg);
		return *this;
	}
	return *this = PostExp(OR,(Exp)*this,(Exp)arg);
}

Smt::PostExp& Smt::PostExp::conj_eq( Smt::PostExp const& arg ) & {
	if( *this == FALSE ) {
		return *this;
	}
	if( *this == TRUE ) {
		Exp::operator=(arg);
		return *this;
	}
	if( fun() == AND ) {
		args().push_back(arg);
		return *this;
	}
	return *this = PostExp(AND,(Exp)*this,(Exp)arg);
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

Smt::PostExp Smt::Solver::declare_const( string_view const& name, BaseSort const& sort ) & {
	_proc.to << "(declare-const " << name << ' ' << sort.name << ')' << endl;
	return name;
}

Smt::PostExp Smt::Solver::define_fun(
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
	assert( _reader.read_sym() == e );
	PostExp ret = _reader.read_exp();
	_reader.close();
	_reader.close();
	return ret;
}

std::string Smt::Solver::_make_fresh() & {
	return string("_") + to_string(_var_count++);
}

Smt::PostExp Smt::car( PostExp const& arg ) {
	if( arg.fun() != CONS ) {
		throw Error{"#car-on",(Exp)arg};
	}
	auto const& args = arg.args();
	assert( args.size() == 2 );
	return args[0];
}
Smt::PostExp Smt::cdr( PostExp const& arg ) {
	if( arg.fun() != CONS ) {
		throw Error{"#cdr-on",(Exp)arg};
	}
	auto const& args = arg.args();
	assert( args.size() == 2 );
	return args[1];
}

Smt::PostExp Smt::PostExp::operator*( PostExp const& arg ) const {
	if( *this == ZERO ) {
		return *this;
	}
	if( *this == ONE || arg == ZERO ) {
		return arg;
	}
	if( arg == ONE ) {
		return *this;
	}
	return Exp{MUL,(Exp)*this,(Exp)arg};
}

Smt::PostExp Smt::Solver::expand( PreExp const& p ) {
	if( auto post = p.post() ) {
		return *post;
	}
	if( auto app = p.app() ) {
		auto const& [fun,args] = *app;
		auto ret = Exp(fun);
		auto& eargs = ret.args();
		if( fun == AND ) {
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
		} else if( fun == OR ) {
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
		} else if( fun == NOT ) {
			assert( args.size() == 1 );
			auto const& earg = expand(args[0]);
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
		} else if( fun == ADD ) {
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
		} else if( fun == MUL ) {
			assert( args.size() == 2 );
			auto const& earg1 = expand(args[0]);
			if( earg1 == ZERO ) {
				return ZERO;
			}
			auto const& earg2 = expand(args[1]);
			if( earg2 == ZERO || earg1 == ONE ) {
				return earg2;
			}
			if( earg2 == ONE ) {
				return earg1;
			}
			if( earg1.fun() == ITE ) {
				auto const& ite_args = earg1.args();
				assert( ite_args.size() == 3 );
				Exp t = PostExp(ite_args[1]) * earg2;
				Exp e = PostExp(ite_args[2]) * earg2;
				return PostExp{ITE,ite_args[0],t,e};
			}
			if( earg2.fun() == ITE ) {
				auto const& ite_args = earg2.args();
				assert( ite_args.size() == 3 );
				Exp t = earg1 * ite_args[1];
				Exp e = earg1 * ite_args[2];
				return PostExp{ITE,ite_args[0],t,e};
			}
			return PostExp{MUL,(Exp)earg1,(Exp)earg2};
		} else if( fun == CAR ) {
			assert( args.size() == 1 );
			return car(expand(args[0]));
		} else if( fun == CDR ) {
			assert( args.size() == 1 );
			return cdr(expand(args[0]));
		} else {
			for( auto const& arg : args ) {
				eargs.push_back(expand(arg));
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
		if( val.args().empty() ) {
			return val;
		}
		auto var = _make_fresh();
		return define_fun(var,{},*base,val);
	}
	auto const& sargs = sort._exp.args();
	assert( val.fun() == CONS );
	auto const& vargs = val.args();
	auto const& v1 = _expand_let(sargs[0],vargs[0]);
	auto const& v2 = _expand_let(sargs[1],vargs[1]);
	return (v1,v2);
};

int Smt::test() try {
	cout << "this is Smt::test()." << endl;
	cout << PreExp(1) + "x" << endl;
	cout << !!(PreExp(0) + []{ return "x"; }) << endl;
	cout << (If("p") ^ PreExp(3) * "x" * "y" ^ ZERO) << endl;
	cout << !(Smt::eq("x","y") && Smt::ge("y",3)) << endl;
	auto z3 = Z3(QF_LIA,cout);
	auto x = z3.declare_const("x","Int");
	auto five = z3.define_fun("five",{},Smt::INT,PostExp(5));
	z3.ass( Smt::gt(x, five + 4) );
	cout << z3.check_sat().result().is_sat() << endl;
	auto xv = z3.get_value(x);
	cout << x << " := " << xv << endl;

	cout << z3.expand( FALSE && []{ return PreExp("BUG"); } ) << endl;

	cout << z3.expand( TRUE || []{ return PreExp("BUG"); } ) << endl;

	cout << z3.expand( If( TRUE ) ^ []{ return PreExp("ok"); } ^ []{ return PreExp("BUG"); } ) << endl;

	cout << z3.expand( If( FALSE ) ^ []{ return PreExp("BUG"); } ^ []{ return PreExp("ok"); } ) << endl;

	cout << z3.expand( If( "cond" ) ^ []{ return PreExp("then"); } ^ []{ return PreExp("else"); } ) << endl;

	z3.ass(
		Let(INT, x + "five") ^ []( PreExp const& x5 ){ return Smt::ge(x5 + x5, "20"); }
	);

	cout << z3.check_sat().result().is_sat() << endl;

	auto car_xy = car((x,"y"));
	cout << car_xy << endl;
	cout << z3.expand(car_xy) << endl;

	auto cons_let = Let((BOOL,INT), (TRUE,x)) ^ []( PreExp const& pair ) { return cdr(pair); };
	cout << cons_let << endl;
	cout << z3.expand(cons_let) << endl;

	return 0;
} catch( ::Exp::Error const& e ) {
	cerr << e << endl;
	return -1;
}
