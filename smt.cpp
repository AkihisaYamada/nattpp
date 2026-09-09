#include<iostream>
#include<cassert>
#include"smt.hpp"
#include"util.hpp"

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

std::string const
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
	Smt::LE = "<=",
	Smt::GT = ">",
	Smt::CONS = "cons",
	Smt::CAR = "car",
	Smt::CDR = "cdr",
	Smt::LIST = "list",
	Smt::NTH = "nth",
	Smt::MAX = "max";

Set<std::string> const Smt::FUNS = {
	TRUE_F, FALSE_F, AND, OR, NOT, IMP, ITE, ADD, MUL, EQ, GE, LE, GT, CONS, CAR, CDR, LIST, NTH
};

Smt::PostExp const Smt::TRUE = PostExp(TRUE_F);
Smt::PostExp const Smt::FALSE = PostExp(FALSE_F);

ostream& operator<<( ostream& os, Smt::Rat const& r ) {
	if( r.denom() == 1 ) {
		return os << r.numen();
	}
	return os << "(/ " << r.numen() << ' ' << r.denom() << ')';
}
ostream& operator<<( ostream& os, Smt::Val const& v ) {
	if( auto const& i = v.is_int() ) {
		if( *i < 0 ) return os << "(- " << *i << ')';
		return os << *i;
	} else if( auto const& r = v.is_rat() ) {
		return os << *r;
	}
	assert(false);
}
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
	if( auto i = f.ref<Smt::Val>() ) {
		return os << *i;
	}
	assert(false);
}

ostream& operator<<( ostream& os, Smt::PreExp const& e ) {
	if( auto post = e.is_post() ) {
		return os << *post;
	}
	if( auto app = e.is_app() ) {
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
	if( auto let = e.is_let() ) {
		auto const& [val,sort,body] = *let;
		return os << "(let " << val << ' ' << sort << " ...)";
	}
	if( auto lazy = e.is_lazy() ) {
		return os << "...";
	}
	assert(false);
}

string to_string( Smt::Fun const& fun ) {
	if( auto i = fun.ref<Smt::Val>() ) {
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
	if( is_app().contains(AND) ) {
		for( auto const& c : _term.args() ) {
			cs.push_back(c);
		}
	} else {
		cs.push_back(_term);
	}
	if( y.is_app().contains(AND) ) {
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
	if( is_app().contains(OR) ) {
		for( auto const& d : _term.args() ) {
			ds.push_back(d);
		}
	} else {
		ds.push_back(_term);
	}
	if( y.is_app().contains(OR) ) {
		for( auto const& d : y._term.args() ) {
			ds.push_back(d);
		}
	} else {
		ds.push_back(y._term);
	}
	return app(OR,std::move(ds));
}

Smt::PostExp Smt::PostExp::imp( Smt::PostExp const& y ) const {
	if( *this == false ) return true;
	if( *this == true ) return y;
	if( y == true ) return true;
	if( y == false ) return !*this;
	if( is_app().contains(NOT) ) {
		assert( _term.args().size() == 1 );
		return _term.arg(0) || y;
	}
	if( y.is_app().contains(OR) ) {
		auto yargs = y._term.args();
		yargs.push_back(!*this);
		return app(OR,std::move(yargs));
	}
	return Term<Fun>(IMP,_term,y._term);
}

Smt::PostExp Smt::eq( PostExp const& x, PostExp const& y ) {
	if( x._term.args().empty() ) {
		if( auto xi = x.is_val() ) {
			if( auto yi = y.is_val() ) {
				return *xi == *yi;
			}
		}
		if( x == y ) {
			return true;
		}
		if( auto iteo = y.is_ite() ) {
			auto const& [i,t,e] = *iteo;
			return ite(i,eq(x,t),eq(x,e));
		}
	} else if( y._term.args().empty() ) {
		if( auto const& iteo = x.is_ite() ) {
			auto const& [i,t,e] = *iteo;
			return ite(i,eq(t,y),eq(e,y));
		}
	}
	return Term<Fun>(EQ,x,y);
}

Smt::PostExp Smt::ge( PostExp const& x, PostExp const& y ) {
	if( x._term.args().empty() ) {
		if( auto xi = x.is_val() ) {
			if( auto yi = y.is_val() ) {
				return *xi >= *yi;
			}
		}
		if( x == y ) {
			return true;
		}
		if( auto iteo = y.is_ite() ) {
			auto const& [i,t,e] = *iteo;
			return ite(i,ge(x,t),ge(x,e));
		}
	} else if( y._term.args().empty() ) {
		if( auto const& iteo = x.is_ite() ) {
			auto const& [i,t,e] = *iteo;
			return ite(i,ge(t,y),ge(e,y));
		}
	}
	return Term<Fun>(GE,x,y);
}

Smt::PostExp Smt::le( PostExp const& x, PostExp const& y ) {
	if( x._term.args().empty() ) {
		if( auto xi = x.is_val() ) {
			if( auto yi = y.is_val() ) {
				return *xi <= *yi;
			}
		}
		if( x == y ) {
			return true;
		}
		if( auto iteo = y.is_ite() ) {
			auto const& [i,t,e] = *iteo;
			return ite(i,le(x,t),le(x,e));
		}
	} else if( y._term.args().empty() ) {
		if( auto const& iteo = x.is_ite() ) {
			auto const& [i,t,e] = *iteo;
			return ite(i,le(t,y),le(e,y));
		}
	}
	return Term<Fun>(LE,x,y);
}

Smt::PostExp Smt::gt( PostExp const& x, PostExp const& y ) {
	if( x._term.args().empty() ) {
		if( auto xi = x.is_val() ) {
			if( auto yi = y.is_val() ) {
				return *xi < *yi;
			}
		}
		if( x == y ) {
			return false;
		}
		if( auto iteo = y.is_ite() ) {
			auto const& [i,t,e] = *iteo;
			return ite(i,gt(x,t),gt(x,e));
		}
	} else if( y._term.args().empty() ) {
		if( auto const& iteo = x.is_ite() ) {
			auto const& [i,t,e] = *iteo;
			return ite(i,gt(t,y),gt(e,y));
		}
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
	if( t == true ) {
		return i || e;
	}
	if( t == false ) {
		return !i && e;
	}
	if( e == true ) {
		return i.imp(t);
	}
	if( e == false ) {
		return i && t;
	}
	if( auto tv = t.is_val() ) {
		if( t == e ) return t;
	}
	if( auto ev = e.is_val() ) {
		if( auto to = t.is_ite() ) {
			auto const& [ti,tt,te] = *to;
			if( te == *ev ) {
				return Term<Fun>(ITE,i&&ti,tt,te);
			}
		}
	}
	return Term<Fun>(ITE,i,t,e);
}
Opt<std::tuple<Smt::PostExp,Smt::PostExp,Smt::PostExp>> Smt::PostExp::is_ite() const & {
	if( is_app().contains(ITE) && this->_term.args().size() == 3 ) {
		return {{this->_term.args()[0],this->_term.args()[1],this->_term.args()[2]}};
	}
	return {};
}

Smt::PreExp& operator+=( Smt::PreExp& x, Smt::PreExp const& y ) {
	if( x.is_post() && []( auto const& e ){ return e == 0; } ) {
		return x = y;
	}
	if( y.is_post() && []( auto const& e ){ return e == 0; } ) {
		return x;
	}
	auto xapp = x._un.ref<Ref<Smt::PreExp::App>>();
	auto const& yapp = y.is_app();
	if( xapp && (**xapp).fun == Smt::ADD ) {
		auto& xargs = (**xapp).args;
		if( yapp && yapp->fun == Smt::ADD ) {
			for( auto const& yarg : yapp->args ) {
				xargs.emplace_back(yarg);
			}
		} else {
			xargs.emplace_back(y);
		}
		return x;
	}
	auto args = std::vector<Smt::PreExp>{x};
	if( yapp && yapp->fun == Smt::ADD ) {
		for( auto const& yarg : yapp->args ) {
			args.emplace_back(yarg);
		}
	} else {
		args.emplace_back(y);
	}
	return x = Smt::PreExp(Smt::ADD,std::move(args));
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

Algebra<string,Smt::PreExp> const Smt::ALGEBRA = []( string const& fun, vector<Smt::PreExp>&& args ){
	return Smt::PreExp(fun,std::move(args));
};

Smt::Solver::Solver( unique_ptr<Proc>&& proc, Logic const& logic, bool use_let ) :
	_status(UNKNOWN), _proc(std::move(proc)), _reader(_proc->from), _var_count(0), _logic(logic), _use_let(use_let)
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
		throw Error("#smt:get_value","\"status is not SAT\"");
	}
	return _get_value(e);
}
Smt::PostExp Smt::Solver::get_value( PreExp const& e ) & {
	if( _status != SAT ) {
		throw Error("#smt:get_value","\"status is not SAT\"");
	}
	return _get_value(e);
}
Smt::PostExp Smt::Solver::_get_value( PostExp const& e ) & {
	if( e.is_app().contains(CONS) ) {
		auto vargs = std::vector<Term<Fun>>();
		for( auto const& arg : e._term.args() ) {
			vargs.emplace_back(_get_value(arg));
		}
		return app(CONS,std::move(vargs));
	}
	return _get_value_base(e);
}
Smt::PostExp Smt::Solver::_get_value( PreExp const& e ) & {
	if( auto const& post = e.is_post() ) {
		return _get_value(*post);
	}
	if( auto const& app = e.is_app() ) {
		auto const& [fun,args] = *app;
		if( fun == MUL ) {
			Val ret = 1;
			for( auto const& arg : args ) {
				auto varg = _get_value(arg);
				Val v = varg.is_val().value_or_throw(Error("#smt:get_value","\"bad value\""));
				ret *= v;
			}
			return ret;
		}
	}
	throw Error("#smt:bad-get_value");
}
Smt::PostExp Smt::Solver::_get_value_base( PostExp const& e ) & {
	_proc->to << "(get-value (" << e << "))" << endl;
	_reader.open();
	_reader.open();
	auto re = _reader.read_post_exp();
	if( re != e ) {
		cerr << re << endl; assert(false);
	}
	PostExp ret = _reader.read_post_exp();
	_reader.close();
	_reader.close();
	return ret;
}

std::string Smt::Solver::_make_fresh() & {
	return string("_") + to_string(_var_count++);
}

Smt::PostExp Smt::car( PostExp const& arg ) {
	if( arg.is_app().contains(CONS) ) {
		auto& args = arg._term.args();
		if( args.size() == 2 ) return args[0];
	}
	throw Error{"#car-on",arg.exp()};
}
Smt::PostExp Smt::cdr( PostExp const& arg ) {
	if( arg.is_app().contains(CONS) ) {
		auto& args = arg._term.args();
		if( args.size() == 2 ) return args[1];
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
	if( is_app().contains(NOT) ) {
		if( _term.args().size() != 1 ) throw Error("#smt:bad-not");
		return _term.arg(0);
	}
	return Term<Fun>(NOT,*this);
}
Smt::PostExp& operator+=( Smt::PostExp& x, Smt::PostExp const& y ) {
	if( auto num = x.is_val() ) {
		if( *num == 0 ) {
			return x = y;
		}
		if( auto num2 = y.is_val() ) {
			return x = *num + *num2;
		}
	} else if( auto num2 = y.is_val() ) {
		if( *num2 == 0 ) {
			return x;
		}
	}
	return x = Term<Smt::Fun>(Smt::ADD,x,y);
}
Smt::PostExp& Smt::PostExp::mul_eq( Smt::PostExp const& y, bool linear ) & {
	if( auto num = is_val() ) {
		if( *num == 0 ) {
			return *this;
		}
		if ( *num == 1 ) {
			return *this = y;
		}
		if( auto num2 = y.is_val() ) {
			return *this = *num * *num2;
		}
	} else if( auto num2 = y.is_val() ) {
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
	if( auto post = p.is_post() ) {
		return *post;
	}
	if( auto o = p.is_app() ) {
		auto const& [fun,args] = *o;
		auto eargs = vector<Term<Fun>>();
		if( fun == AND ) {
			PostExp ret = true;
			for( auto const& arg : args ) {
				ret = ret.conj(expand(arg));
				if( ret == false ) break;
			}
			return ret;
		}
		if( fun == OR ) {
			PostExp ret = false;
			for( auto const& arg : args ) {
				ret = ret.disj(expand(arg));
				if( ret == true ) break;
			}
			return ret;
		}
		if( fun == NOT ) {
			assert( args.size() == 1 );
			return !expand(args[0]);
		}
		if( fun == IMP ) {
			assert( args.size() == 2 );
			auto const& x = expand(args[0]);
			if( x == false ) return true;
			auto const& y = expand(args[1]);
			return x.imp(y);
		}
		if( fun == ITE ) {
			assert( args.size() == 3 );
			auto const& i = expand(args[0]);
			if( i == TRUE ) {
				return expand(args[1]);
			}
			if( i == FALSE ) {
				return expand(args[2]);
			}
			return ite(i,expand(args[1]),expand(args[2]));
		}
		if( fun == EQ ) {
			assert( args.size() == 2 );
			auto earg1 = expand(args[0]), earg2 = expand(args[1]);
			return eq(earg1,earg2);
		}
		if( fun == GE ) {
			assert( args.size() == 2 );
			auto earg1 = expand(args[0]), earg2 = expand(args[1]);
			return ge(expand(args[0]),expand(args[1]));
		}
		if( fun == LE ) {
			assert( args.size() == 2 );
			return le(expand(args[0]),expand(args[1]));
		}
		if( fun == GT ) {
			assert( args.size() == 2 );
			return gt(expand(args[0]),expand(args[1]));
		}
		if( fun == ADD ) {
			for( auto const& arg : args ) {
				auto const& earg = expand(arg);
				if( auto const& oi = earg.is_val() ) {
					if( *oi == 0 ) {
						continue;
					}
				} else if( earg.is_app().contains(ADD) ) {
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
			if( auto num1 = earg1.is_val() ) {
				if( *num1 == 0 ) return 0;
				if( *num1 == 1 ) return expand(args[1]);
			}
			auto const& earg2 = expand(args[1]);
			if( auto num2 = earg2.is_val() ) {
				if( *num2 == 0 ) return 0;
				if( *num2 == 1 ) return earg1;
			}
			if( logic().linear() ) {
				if( earg1.is_app().contains(ITE) ) {
					auto const& eargs1 = earg1._term.args();
					assert( eargs1.size() == 3 );
					auto v = let(logic().base_sort(),earg2);//TODO
					return ite( eargs1[0], Smt::mul(eargs1[1],v,true), Smt::mul(eargs1[2],v,true) );
				}
				if( earg2.is_app().contains(ITE) ) {
					auto const& eargs2 = earg2._term.args();
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
	if( auto tp = p.is_let() ) {
		auto const& [val,sort,body] = *tp;
		return expand(body(let(sort,expand(val))));
	}
	if( auto lazy = p.is_lazy() ) {
		return expand((*lazy)(*this));
	}
	assert(false);
};
Smt::PostExp Smt::Solver::let( Sort const& sort, PostExp const& val ) & {
	if( !_use_let || val.is_val() ) {
		return val;
	}
	auto vfun = *val.is_app();
	auto const& vargs = val._term.args();
	if( _logic._linear ) {
		if( vfun == MUL ) {
			return val;
		}
		if( vfun == ITE ) {
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
	Opt<bool> let;
	Exp::KeyValProc proc_tee_key = [&]( auto key, auto val ){
		if( key == "tee" ) {
			tee.emplace(OStream::of(val));
			return true;
		}
		return false;
	};
	Exp::KeyValProc proc_let_key = [&]( auto key, auto val ){
		if( key == "let" ) {
			if( let ) throw Error("#duplicate-key",key);
			let = {val.as_bool()};
			return true;
		}
		return false;
	};
	if( x.fun() == "z3" ) {
		auto logic = Logic::of(x.get_arg(n));
		x.process_keys( n, proc_tee_key || proc_let_key );
		return Z3(logic,std::move(tee),let.value_or(true));
	}
	if( x.fun() == "cvc5" ) {
		auto logic = Logic::of(x.get_arg(n));
		x.process_keys( n, proc_tee_key || proc_let_key );
		return CVC5(logic,std::move(tee),let.value_or(true));
	}
	throw Error("#malformed-smt-solver",x);
}

int Smt::test() try {
	cout << "this is Smt::test()." << endl;
	cout << 1 + PostExp("x") << endl;
	cout << !!(PostExp(0) + [](auto&){ return PostExp("x"); }) << endl;
	cout << ite(PostExp("p"), PostExp(3) * PostExp("x") * PostExp("y"), PostExp(0)) << endl;
	cout << !(Smt::eq(PostExp("x"),PostExp("y")) && Smt::ge(PostExp("y"),3)) << endl;
	auto z3 = Smt::Solver::of({"z3","QF_LIA",":tee","cout"});
	auto x = z3.declare_const("x","Int");
	auto y = z3.define_fun("y",{},Smt::INT,5);
	z3.ass( Smt::gt( x, y + 4 ) );
	bool sat = z3.check_sat().result().is_sat();
	assert(sat);
	auto xv = z3.get_value(x);
	cout << x << " := " << xv << endl;
	auto yv = z3.get_value(y);
	cout << y << " := " << yv << endl;
	assert( xv.as_val() > yv.as_val() + 4);

	cout << z3.expand( FALSE && [](auto&){ return PostExp("BUG"); } ) << endl;

	cout << z3.expand( TRUE || [](auto&){ return PostExp("BUG"); } ) << endl;

	cout << z3.expand( If( TRUE ) ^ [](auto&){ return PostExp("ok"); } ^ [](auto&){ return PostExp("BUG"); } ) << endl;

	cout << z3.expand( If( FALSE ) ^ [](auto&){ return PostExp("BUG"); } ^ [](auto&){ return PostExp("ok"); } ) << endl;

	cout << z3.expand( If( PostExp("cond") ) ^ [](auto&){ return PostExp("then"); } ^ [](auto&){ return PostExp("else"); } ) << endl;

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
