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

Smt::PostExp const Smt::TRUE = PostExp("true");
Smt::PostExp const Smt::FALSE = PostExp("false");

ostream& operator<<( ostream& os, Smt::Sort const& e ) {
	if( auto base = e.base() ) {
		return os << *base;
	}
	if( auto cons = e.cons() ) {
		auto const& [e1,e2] = *cons;
		return os << "(cons " << e1 << ' ' << e2 << ')';
	}
	assert(false);
}

ostream& operator<<( ostream& os, Smt::PostExp const& e ) {
	if( auto num = e.num() ) {
		return os << *num;
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
	assert(false);
}
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
Smt::PostExp Smt::PostExp::conj( Smt::PostExp const& y ) const {
	if( *this == FALSE || y == TRUE ) {
		return *this;
	}
	if( *this == TRUE || y == FALSE ) {
		return y;
	}
	return PostExp(AND,{*this,y});
}
Smt::PostExp Smt::PostExp::disj( Smt::PostExp const& y ) const {
	if( *this == TRUE || y == FALSE ) {
		return *this;
	}
	if( *this == FALSE || y == TRUE ) {
		return y;
	}
	return PostExp(OR,{*this,y});
}

Smt::PostExp& Smt::PostExp::disj_eq( Smt::PostExp const& arg ) & {
	if( auto app = this->app() ) {
		auto& [fun,args] = *app;
		if( args.empty() ) {
			if( fun == "true" ) {
				return *this;
			}
			if( fun == "false" ) {
				return *this = arg;
			}
		}
		if( fun == "or" ) {
			args.push_back(arg);
			return *this;
		}
	}
	return *this = PostExp(OR,{*this,arg});
}

Smt::PostExp& Smt::PostExp::conj_eq( Smt::PostExp const& arg ) & {
	if( auto app = this->app() ) {
		auto& [fun,args] = *app;
		if( args.empty() ) {
			if( fun == "false" ) {
				return *this;
			}
			if( fun == "true" ) {
				return *this = arg;
			}
		}
		if( fun == "and" ) {
			args.push_back(arg);
			return *this;
		}
	}
	return *this = PostExp(AND,{*this,arg});
}

Smt::PostExp Smt::eq( PostExp const& x, PostExp const& y ) {
	if( auto xi = x.num() ) {
		if( auto yi = y.num() ) {
			return xi == yi ? TRUE : FALSE;
		}
	}
	return PostExp(EQ,{x,y});
}

Smt::PostExp Smt::ge( PostExp const& x, PostExp const& y ) {
	if( auto xi = x.num() ) {
		if( auto yi = y.num() ) {
			return xi >= yi ? TRUE : FALSE;
		}
	}
	return PostExp(GE,{x,y});
}

Smt::PostExp Smt::gt( PostExp const& x, PostExp const& y ) {
	if( auto xi = x.num() ) {
		if( auto yi = y.num() ) {
			return xi > yi ? TRUE : FALSE;
		}
	}
	return PostExp(GT,{x,y});
}

Smt::PostExp Smt::ite( PostExp const& i, PostExp const& t, PostExp const& e ) {
	if( i == TRUE ) {
		return t;
	}
	if( i == FALSE ) {
		return e;
	}
	return PostExp(ITE,{i,t,e});
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
	return PostExp(name);
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
	return PostExp(name);
}

Smt::PostExp Smt::Reader::read_post_exp() {
	if( auto sym = reads_sym() ) {
		return Smt::PostExp(*sym);
	}
	if( opens() ) {
		auto fun = reads_sym();
		if( !fun ) {
			throw Error({"#smt:read",*fun});
		}
		vector<PostExp> args;
		while( !closes() ) {
			args.push_back(read_post_exp());
		}
		return PostExp(std::move(*fun),std::move(args));
	}
	throw Error({"#smt:read"});
}
Smt::PostExp Smt::Solver::get_value( PostExp const& e ) & {
	if( _status != SAT ) {
		throw Error("#smt:get_value");
	}
	_proc.to << "(get-value (" << e << "))" << endl;
	_reader.open();
	_reader.open();
	auto re = _reader.read_post_exp();
	assert( re == e );
	PostExp ret = _reader.read_post_exp();
	_reader.close();
	_reader.close();
	return ret;
}

std::string Smt::Solver::_make_fresh() & {
	return string("_") + to_string(_var_count++);
}

Smt::PostExp Smt::car( PostExp const& arg ) {
	if( auto const& app = arg.app() ) {
		auto const& [fun,args] = *app;
		if( fun == CONS && args.size() == 2 ) {
			return args[0];
		}
	}
	throw Error{"#car-on",arg.exp()};
}
Smt::PostExp Smt::cdr( PostExp const& arg ) {
	if( auto const& app = arg.app() ) {
		auto const& [fun,args] = *app;
		if( fun == CONS && args.size() == 2 ) {
			return args[1];
		}
	}
	throw Error{"#cdr-on",arg.exp()};
}
Smt::PostExp Smt::PostExp::operator!() const {
	if( auto const& app = this->app() ) {
		auto const& [fun,args] = *app;
		if( fun == "true" ) {
			return FALSE;
		}
		if( fun == "false" ) {
			return TRUE;
		}
		if( fun == NOT ) {
			return args[0];
		}
	}
	return PostExp(NOT,{*this});
}
Smt::PostExp Smt::PostExp::add( PostExp const& arg ) const {
	if( auto num = this->num() ) {
		if( *num == 0 ) {
			return arg;
		}
		if( auto num2 = arg.num() ) {
			return *num + *num2;
		}
	} else if( auto num2 = arg.num() ) {
		if( num2 == 0 ) {
			return *this;
		}
	}
	return PostExp(ADD,{*this,arg});
}

Smt::PostExp& Smt::PostExp::operator+=( PostExp const& arg ) & {
	if( auto num = this->num() ) {
		if( *num == 0 ) {
			return *this = arg;
		}
		if( auto num2 = arg.num() ) {
			*num += *num2;
			return *this;
		}
	} else if( auto num2 = arg.num() ) {
		if( num2 == 0 ) {
			return *this;
		}
	}
	return *this = PostExp(ADD,{*this,arg});
}

Smt::PostExp Smt::PostExp::mul( PostExp const& arg ) const {
	if( auto num = this->num() ) {
		if( *num == 0 ) {
			return *this;
		}
		if ( *num == 1 ) {
			return arg;
		}
		if( auto num2 = arg.num() ) {
			return *num * *num2;
		}
	} else if( auto num2 = arg.num() ) {
		if( num2 == 0 ) {
			return arg;
		}
		if( num2 == 1 ) {
			return *this;
		}
	}
	return PostExp(MUL,{*this,arg});
}

Smt::PostExp& Smt::PostExp::operator*=( PostExp const& arg ) & {
	if( auto num = this->num() ) {
		if( *num == 0 ) {
			return *this;
		}
		if ( *num == 1 ) {
			return *this = arg;
		}
		if( auto num2 = arg.num() ) {
			*num *= *num2;
			return *this;
		}
	} else if( auto num2 = arg.num() ) {
		if( num2 == 0 ) {
			return *this = arg;
		}
		if( num2 == 1 ) {
			return *this;
		}
	}
	return *this = PostExp(MUL,{*this,arg});
}
Exp Smt::PostExp::exp() const {
	if( auto num = this->num() ) {
		return to_string(*num);
	}
	if( auto app = this->app() ) {
		auto const& [fun,args] = *app;
		Exp ret = fun;
		for( auto& arg : args ) {
			ret.args().push_back(arg.exp());
		}
		return ret;
	}
	assert(false);
}
Smt::PostExp Smt::Solver::expand( PreExp const& p ) {
	if( auto post = p.post() ) {
		return *post;
	}
	if( auto app = p.app() ) {
		auto const& [fun,args] = *app;
		auto eargs = vector<PostExp>();
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
			return !expand(args[0]);
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
				if( earg != 0 ) {
					eargs.push_back(earg);
				}
			}
			switch( eargs.size() ) {
				case 0: return 0;
				case 1: return eargs[0];
			}
		} else if( fun == MUL ) {
			assert( args.size() == 2 );
			auto const& earg1 = expand(args[0]);
			if( auto num1 = earg1.num() ) {
				if( *num1 == 0 ) {
					return 0;
				}
				if( *num1 == 1 ) {
					return expand(args[1]);
				}
			}
			auto const& earg2 = expand(args[1]);
			if( auto num2 = earg2.num() ) {
				if( *num2 == 0 ) {
					return 0;
				}
				if( *num2 == 1 ) {
					return earg1;
				}
			}
			if( auto eapp1 = earg1.app() ) {
				auto const& [efun1,eargs1] = *eapp1;
				if( efun1 == ITE ) {
					assert( eargs1.size() == 3 );
					auto v = let(INT,earg2);//TODO
					return PostExp( ITE, { eargs1[0], eargs1[1] * v, eargs1[2] * v } );
				}
			}
			if( auto eapp2 = earg2.app() ) {
				auto const& [efun2,eargs2] = *eapp2;
				if( efun2 == ITE ) {
					assert( eargs2.size() == 3 );
					auto v = let(INT,earg1);//TODO
					return PostExp( ITE, {eargs[0], v * eargs[1], v * eargs[2]} );
				}
			}
			return PostExp(MUL,{earg1,earg2});
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
		return PostExp(fun,std::move(eargs));
	}
	if( auto tp = p.let() ) {
		auto const& [val,sort,body] = *tp;
		return expand(body(let(sort,expand(val))));
	}
	if( auto lazy = p.lazy() ) {
		return expand((*lazy)());
	}
	assert(false);
};
Smt::PostExp Smt::Solver::let( Sort const& sort, PostExp const& val ) & {
	if( auto const& base = sort.base() ) {
		if( auto app = val.app() ) {
			if( !app->second.empty() ) {
				auto var = _make_fresh();
				return define_fun(var,{},*base,val);
			}
		}
		return val;
	}
	if( auto const& cons = sort.cons() ) {
		auto const& [sort1,sort2] = *cons;
		auto app = val.app();
		assert(app);
		auto const& [fun,vargs] = *app;
		assert( fun == CONS );
		auto const& v1 = let(sort1,vargs[0]);
		auto const& v2 = let(sort2,vargs[1]);
		return (v1,v2);
	}
	assert(false);
};

int Smt::test() try {
	cout << "this is Smt::test()." << endl;
	cout << 1 + PostExp("x") << endl;
	cout << !!(PostExp(0) + []{ return PostExp("x"); }) << endl;
	cout << ite(PostExp("p"), 3 * PostExp("x") * PostExp("y"), 0) << endl;
	cout << !(Smt::eq(PostExp("x"),PostExp("y")) && Smt::ge(PostExp("y"),3)) << endl;
	auto z3 = Z3(QF_LIA,cout);
	auto x = z3.declare_const("x","Int");
	auto y = z3.define_fun("y",{},Smt::INT,5);
	z3.ass( Smt::gt( x, y + 4 ) );
	cout << z3.check_sat().result().is_sat() << endl;
	auto xv = z3.get_value(x);
	cout << x << " := " << xv << endl;

	cout << z3.expand( FALSE && []{ return PostExp("BUG"); } ) << endl;

	cout << z3.expand( TRUE || []{ return PostExp("BUG"); } ) << endl;

	cout << z3.expand( If( TRUE ) ^ []{ return PostExp("ok"); } ^ []{ return PostExp("BUG"); } ) << endl;

	cout << z3.expand( If( FALSE ) ^ []{ return PostExp("BUG"); } ^ []{ return PostExp("ok"); } ) << endl;

	cout << z3.expand( If( PostExp("cond") ) ^ []{ return PostExp("then"); } ^ []{ return PostExp("else"); } ) << endl;

	z3.ass(
		Let(INT, x + y) ^ []( PostExp const& x5 ){ return Smt::ge(x5 + x5, 20); }
	);

	cout << z3.check_sat().result().is_sat() << endl;

	auto car_xy = car((x,PostExp("y")));
	cout << car_xy << endl;
	cout << z3.expand(car_xy) << endl;

	auto cons_let = Let((BOOL,INT), (TRUE,x)) ^ []( PreExp const& pair ) { return cdr(pair); };
	cout << cons_let << endl;
	cout << z3.expand(cons_let) << endl;

	return 0;
} catch( ::Error const& e ) {
	cerr << e << endl;
	return -1;
}
