#include<iostream>
#include<cassert>
#include"smt.hpp"

using namespace std;

Smt::BaseSort const Smt::BOOL = "Bool";
Smt::BaseSort const Smt::INT = "Int";
Smt::BaseSort const Smt::REAL = "Real";

Smt::Logic const Smt::QF_LIA = {"QF_LIA",true,Smt::INT};
Smt::Logic const Smt::QF_LRA = {"QF_LRA",true,Smt::REAL};
Smt::Logic const Smt::LIA = {"LIA",true,Smt::INT};
Smt::Logic const Smt::LRA = {"LRA",true,Smt::REAL};
Smt::Logic const Smt::QF_NIA = {"QF_NIA",false,Smt::INT};
Smt::Logic const Smt::QF_NRA = {"QF_NRA",false,Smt::REAL};
Smt::Logic const Smt::NIA = {"NIA",false,Smt::INT};
Smt::Logic const Smt::NRA = {"NRA",false,Smt::REAL};

Smt::Fun const
	Smt::TRUE_F = "true",
	Smt::FALSE_F = "false",
	Smt::AND = "and",
	Smt::OR = "or",
	Smt::IMP = "=>",
	Smt::NOT = "not",
	Smt::ITE = "ite",
	Smt::ADD = "+",
	Smt::MUL = "*",
	Smt::EQ = "=",
	Smt::GE = ">=",
	Smt::GT = ">",
	Smt::CONS = "cons",
	Smt::CAR = "car",
	Smt::CDR = "cdr",
	Smt::LIST = "list",
	Smt::NTH = "nth";

Smt::PostExp const Smt::TRUE = PostExp(TRUE_F);
Smt::PostExp const Smt::FALSE = PostExp(FALSE_F);

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
std::ostream& operator<<( std::ostream& os, Smt::Fun const& f ) {
	if( auto str = f.ref<string>() ) {
		return os << *str;
	}
	if( auto i = f.ref<int>() ) {
		return os << *i;
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

string to_string( Smt::Fun const& fun ) {
	if( auto i = fun.ref<int>() ) {
		return to_string(*i);
	}
	if( auto s = fun.ref<string>() ) {
		return *s;
	}
	assert(false);
}

Exp Smt::PostExp::exp() const {
	return _term.map<string>(
		[](Smt::Fun const& fun){
		return to_string(fun);
	});
}

Smt::PostExp Smt::PostExp::conj( Smt::PostExp const& y ) const& {
	if( *this == TRUE || y == FALSE ) return y;
	if( *this == FALSE || y == TRUE ) return *this;
	std::vector<Term<Fun>> cs;
	if( _term.fun() == AND ) {
		for( auto const& c : _term.args() ) {
			cs.push_back(c);
		}
	} else {
		cs.push_back(_term);
	}
	if( y._term.fun() == AND ) {
		for( auto const& c : y._term.args() ) {
			cs.push_back(c);
		}
	} else {
		cs.push_back(y._term);
	}
	return app(AND,std::move(cs));
}

Smt::PostExp Smt::PostExp::disj( Smt::PostExp const& y ) const& {
	if( *this == TRUE || y == FALSE ) return *this;
	if( *this == FALSE || y == TRUE ) return y;
	std::vector<Term<Fun>> ds;
	if( _term.fun() == OR ) {
		for( auto const& d : _term.args() ) {
			ds.push_back(d);
		}
	} else {
		ds.push_back(_term);
	}
	if( y._term.fun() == OR ) {
		for( auto const& d : y._term.args() ) {
			ds.push_back(d);
		}
	} else {
		ds.push_back(y._term);
	}
	return app(OR,std::move(ds));
}

Smt::PostExp Smt::PostExp::imp( Smt::PostExp const& y ) const {
	if( _term.fun() == NOT ) {
		return *this || y;
	}
	if( *this == TRUE || y == FALSE ) {
		return y;
	}
	if( *this == FALSE ) {
		return TRUE;
	}
	auto& yfun = y._term.fun();
	auto yargs = y._term.args();
	if( yfun == OR ) {
		yargs.push_back(!*this);
		return app(OR,std::move(yargs));
	}
	return Term<Fun>(IMP,_term,y._term);
}

Smt::PostExp Smt::eq( PostExp const& x, PostExp const& y ) {
	if( auto xi = x.is_int() )
		if( auto yi = y.is_int() ) {
			return *xi == *yi;
		}
	return Term<Fun>(EQ,x,y);
}

Smt::PostExp Smt::ge( PostExp const& x, PostExp const& y ) {
	if( auto xi = x.is_int() )
		if( auto yi = y.is_int() ) {
			return *xi >= *yi;
		}
	return Term<Fun>(GE,x,y);
}

Smt::PostExp Smt::gt( PostExp const& x, PostExp const& y ) {
	if( auto xi = x.is_int() )
		if( auto yi = y.is_int() ) {
			return *xi > *yi;
		}
	return Term<Fun>(GT,x,y);
}

Smt::PostExp Smt::ite( PostExp const& i, PostExp const& t, PostExp const& e ) {
	if( i == TRUE ) {
		return t;
	}
	if( i == FALSE ) {
		return e;
	}
	return Term<Fun>(ITE,i,t,e);
}
Opt<std::tuple<Smt::PostExp,Smt::PostExp,Smt::PostExp>> Smt::PostExp::is_ite() const & {
	if( _term.fun() == ITE && this->_term.args().size() == 3 ) {
		return {{this->_term.args()[0],this->_term.args()[1],this->_term.args()[2]}};
	}
	return {};
}

Smt::BaseSort Smt::BaseSort::of( Exp const& exp ) {
	if( exp == "Int" ) {
		return Smt::INT;
	}
	if( exp == "Bool" ) {
		return Smt::BOOL;
	}
	if( exp == "Real" ) {
		return Smt::REAL;
	}
	throw Error("#unknown-base-sort",exp);
}

Algebra::Intp<string,Smt::PreExp> const Smt::ALGEBRA = []( string const& fun, vector<Smt::PreExp>&& args ){
	return Smt::PreExp(fun,std::move(args));
};

Smt::Solver::Solver( unique_ptr<Proc>&& proc, Logic const& logic ) :
	_status(UNKNOWN), _proc(std::move(proc)), _reader(_proc->from), _var_count(0), _logic(logic)
{
	_proc->to << "(set-logic " << logic._str << ')' << endl;
}

Smt::Solver& Smt::Solver::ass( PostExp const& e ) & {
	if( _status != UNSAT ) {
		_proc->to << "(assert " << e << ')' << endl;
		_status = UNKNOWN;
	}
	return *this;
}

Smt::Solver& Smt::Solver::push() & {
	_proc->to << "(push)" << endl;
	_status = UNKNOWN;
	return *this;
}

Smt::Solver& Smt::Solver::pop() & {
	_proc->to << "(pop)" << endl;
	_status = UNKNOWN;
	return *this;
}

Smt::Solver& Smt::Solver::check_sat() & {
	if( _status == UNKNOWN ) {
		_proc->to << "(check-sat)" << endl;
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

Smt::PostExp Smt::Solver::declare_const( string const& name, BaseSort const& sort ) & {
	_proc->to << "(declare-const " << name << ' ' << sort._name << ')' << endl;
	return Term<Fun>(Fun(in_place_type<string>,name));
}

Smt::PostExp Smt::Solver::define_fun(
	std::string const& name,
	std::initializer_list<std::pair<std::string,std::string>> const& params,
	BaseSort const& sort,
	PostExp const& body
) & {
	_proc->to << "(define-fun " << name << " (";
	for( auto [var,psort] : params ) {
		_proc->to << '(' << var << ' ' << psort << ") ";
	}
	_proc->to << ") " << sort._name << ' ' << body << ')' << endl;
	return Term<Fun>(Fun(in_place_type<string>,name));
}

Smt::PostExp Smt::Reader::read_post_exp() {
	if( auto sym = reads_sym() ) {
		if( auto num = nat_of(*sym) ) {
			return Smt::PostExp(*num);
		}
		return Smt::PostExp(*sym);
	}
	if( opens() ) {
		auto fun = reads_sym();
		if( !fun ) {
			throw Error("#smt:read",*fun);
		}
		vector<Term<Fun>> args;
		while( !closes() ) {
			args.push_back(read_post_exp());
		}
		return Term<Fun>(in_place,std::move(*fun),std::move(args));
	}
	throw Error("#smt:read");
}
Smt::PostExp Smt::Solver::get_value( PostExp const& e ) & {
	if( _status != SAT ) {
		throw Error("#smt:get_value");
	}
	_proc->to << "(get-value (" << e << "))" << endl;
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
	auto& fun = arg._term.fun();
	auto& args = arg._term.args();
	if( fun == CONS && args.size() == 2 ) {
		return args[0];
	}
	throw Error{"#car-on",arg.exp()};
}
Smt::PostExp Smt::cdr( PostExp const& arg ) {
	auto& fun = arg._term.fun();
	auto& args = arg._term.args();
	if( fun == CONS && args.size() == 2 ) {
		return args[1];
	}
	throw Error{"#cdr-on",arg.exp()};
}
Smt::PostExp Smt::PostExp::operator!() const {
	if( *this == TRUE ) {
		return FALSE;
	}
	if( *this == FALSE ) {
		return TRUE;
	}
	auto& fun = _term.fun();
	auto& args = _term.args();
	if( fun == NOT ) {
		return args[0];
	}
	return Term<Fun>(NOT,*this);
}
Smt::PostExp& Smt::PostExp::operator+=( PostExp const& arg ) & {
	if( auto num = this->is_int() ) {
		if( *num == 0 ) {
			return *this = arg;
		}
		if( auto num2 = arg.is_int() ) {
			return *this = *num + *num2;
		}
	} else if( auto num2 = arg.is_int() ) {
		if( *num2 == 0 ) {
			return *this;
		}
	}
	return *this = Term<Fun>(ADD,*this,arg);
}
Smt::PostExp& Smt::PostExp::mul_eq( Smt::PostExp const& y, bool linear ) & {
	if( auto num = is_int() ) {
		if( *num == 0 ) {
			return *this;
		}
		if ( *num == 1 ) {
			return *this = y;
		}
		if( auto num2 = y.is_int() ) {
			return *this = *num * *num2;
		}
	} else if( auto num2 = y.is_int() ) {
		if( *num2 == 0 ) {
			return *this = 0;
		}
		if( *num2 == 1 ) {
			return *this;
		}
	}
	if( linear ) {
		if( auto const& ite = is_ite() ) {
			auto const& [i,t,e] = *ite;
			_term = Term<Fun>(ITE,i,Smt::mul(t,y,true),Smt::mul(e,y,true));
			return *this;
		}
		if( auto const& ite = y.is_ite() ) {
			auto const& [i,t,e] = *ite;
			_term = Term<Fun>(ITE,i,Smt::mul(*this,t,true),Smt::mul(*this,e,true));
			return *this;
		}
	}
	return *this = Term<Fun>(MUL,*this,y);
}

Smt::PostExp Smt::Solver::expand( PreExp const& p ) {
	if( auto post = p.post() ) {
		return *post;
	}
	if( auto o = p.app() ) {
		auto const& [fun,args] = *o;
		auto eargs = vector<Term<Fun>>();
		if( fun == AND ) {
			for( auto const& arg : args ) {
				auto const& earg = expand(arg);
				if( earg == FALSE ) return FALSE;
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
				if( earg == TRUE ) return TRUE;
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
		} else if( fun == EQ ) {
			assert( args.size() == 2 );
			return eq(expand(args[0]),expand(args[1]));
		} else if( fun == GE ) {
			assert( args.size() == 2 );
			return ge(expand(args[0]),expand(args[1]));
		} else if( fun == GT ) {
			assert( args.size() == 2 );
			return gt(expand(args[0]),expand(args[1]));
		} else if( fun == ADD ) {
			for( auto const& arg : args ) {
				auto const& earg = expand(arg);
				auto& afun = earg._term.fun();
				if( afun == Fun(0) ) {
					continue;
				}
				if( afun == ADD ) {
					for( auto& eaarg : earg._term.args() ) {
						eargs.push_back(eaarg);
					}
					continue;
				}
				eargs.push_back(earg);
			}
			switch( eargs.size() ) {
				case 0: return 0;
				case 1: return eargs[0];
			}
		} else if( fun == MUL ) {
			assert( args.size() == 2 );
			auto const& earg1 = expand(args[0]);
			auto const& efun1 = earg1._term.fun();
			if( auto num1 = efun1.ref<int>() ) {
				if( *num1 == 0 ) return 0;
				if( *num1 == 1 ) return expand(args[1]);
			}
			auto const& earg2 = expand(args[1]);
			auto const& efun2 = earg2._term.fun();
			if( auto num2 = efun2.ref<int>() ) {
				if( *num2 == 0 ) return 0;
				if( *num2 == 1 ) return earg1;
			}
			auto const& eargs1 = earg1._term.args();
			if( logic().linear() ) {
				if( efun1 == ITE ) {
					assert( eargs1.size() == 3 );
					auto v = let(logic().base_sort(),earg2);//TODO
					return ite( eargs1[0], Smt::mul(eargs1[1],v,true), Smt::mul(eargs1[2],v,true) );
				}
				auto const& eargs2 = earg2._term.args();
				if( efun2 == ITE ) {
					assert( eargs2.size() == 3 );
					auto v = let(logic().base_sort(),earg1);//TODO
					return ite( eargs2[0], Smt::mul(v,eargs2[1],true), Smt::mul(v,eargs2[2],true) );
				}
			}
			return Term<Fun>(MUL,earg1,earg2);
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
		return app(fun,std::move(eargs));
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
	auto const& vfun = val._term.fun();
	if( vfun.ref<int>() ) {
		return val;
	}
	auto const& vargs = val._term.args();
	if( _logic._linear ) {
		if( vfun.ref<string>().contains("*") ) {
			return val;
		}
		if( vfun.ref<string>().contains("ite") ) {
			auto i = let(BOOL,vargs[0]);
			auto t = let(sort,vargs[1]);
			auto e = let(sort,vargs[2]);
			return Smt::ite(i,t,e);
		}
	}
	if( auto const& base = sort.base() ) {
		if( !vargs.empty() ) {
			auto var = _make_fresh();
			return define_fun(var,{},*base,val);
		}
		return val;
	}
	if( auto const& cons = sort.cons() ) {
		auto const& [sort1,sort2] = *cons;
		assert( vfun == CONS );
		auto const& v1 = let(sort1,vargs[0]);
		auto const& v2 = let(sort2,vargs[1]);
		return (v1,v2);
	}
	assert(false);
};

Smt::Logic Smt::Logic::of( Exp const& x ) {
	if( auto c = x.unapplied() ) {
		if( *c == "QF_LIA" ) return QF_LIA;
		if( *c == "QF_LRA" ) return QF_LRA;
		if( *c == "LIA" ) return LIA;
		if( *c == "LRA" ) return LRA;
		if( *c == "QF_NIA" ) return QF_NIA;
		if( *c == "QF_NRA" ) return QF_NRA;
		if( *c == "NIA" ) return NIA;
		if( *c == "NRA" ) return NRA;
	}
	throw Error("#unknown-logic",x);
}
Smt::Sort Smt::Sort::of( Exp const& x ) {
	auto const& f = x.fun();
	auto const& n = x.args().size();
	if( n == 0 ) {
		if( f == "Bool" ) return BOOL;
		if( f == "Int" ) return INT;
		if( f == "Real" ) return REAL;
	} else if( f == "tuple" ) {
		size_t i = n - 1;
		Sort ret = of(x.arg(i));
		while( i > 0 ) {
			i--;
			ret = Sort(ret,of(x.arg(i)));
		}
	}
	throw Error("#malformed-sort",x);
}

Smt::Solver Smt::Solver::of( Exp const& x ) {
	size_t n = 0;
	Opt<OStream> tee;
	if( x.fun() == "z3" ) {
		auto logic = Logic::of(x.get_arg(n));
		x.process_keys(n,[&]( auto key, auto val ){
			if( key == "tee" ) {
				tee.emplace(OStream::of(val));
				return true;
			}
			return false;
		});
		return Z3(logic,std::move(tee));
	}
	throw Error("#malformed-smt-solver",x);
}

int Smt::test() try {
	cout << "this is Smt::test()." << endl;
	cout << 1 + PostExp("x") << endl;
	cout << !!(PostExp(0) + []{ return PostExp("x"); }) << endl;
	cout << ite(PostExp("p"), PostExp(3) * PostExp("x") * PostExp("y"), PostExp(0)) << endl;
	cout << !(Smt::eq(PostExp("x"),PostExp("y")) && Smt::ge(PostExp("y"),3)) << endl;
	auto z3 = Smt::Solver::of({"z3","QF_LIA"});
	auto x = z3.declare_const("x","Int");
	auto y = z3.define_fun("y",{},Smt::INT,5);
	z3.ass( Smt::gt( x, y + 4 ) );
	bool sat = z3.check_sat().result().is_sat();
	assert(sat);
	auto xv = z3.get_value(x);
	cout << x << " := " << xv << endl;
	auto yv = z3.get_value(y);
	cout << y << " := " << yv << endl;
	assert( xv.as_int() > yv.as_int() + 4);

	cout << z3.expand( FALSE && []{ return PostExp("BUG"); } ) << endl;

	cout << z3.expand( TRUE || []{ return PostExp("BUG"); } ) << endl;

	cout << z3.expand( If( TRUE ) ^ []{ return PostExp("ok"); } ^ []{ return PostExp("BUG"); } ) << endl;

	cout << z3.expand( If( FALSE ) ^ []{ return PostExp("BUG"); } ^ []{ return PostExp("ok"); } ) << endl;

	cout << z3.expand( If( PostExp("cond") ) ^ []{ return PostExp("then"); } ^ []{ return PostExp("else"); } ) << endl;

	z3.ass(
		Let(INT, x + y) ^ []( PostExp const& x5 ){ return Smt::ge(x5 + x5, 20); }
	);
	sat = z3.check_sat().result().is_sat();
	assert(sat);

	auto car_xy = car((x,PostExp("y")));
	cout << car_xy << endl;
	cout << z3.expand(car_xy) << endl;

	auto cons_let = Let((BOOL,INT), (TRUE,x)) ^ []( PreExp const& pair ) { return cdr(pair); };
	cout << cons_let << endl;
	cout << z3.expand(cons_let) << endl;

	auto xy = PreExp(x) * ite(y,0,1);
	cout << xy << "  -->  " << z3.expand(xy) << endl;

	cout << "--- Smt::test done ---" << endl;
	return 0;
} catch( ::Error const& e ) {
	cerr << e << endl;
	return -1;
}
