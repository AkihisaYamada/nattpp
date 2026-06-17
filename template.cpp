#include"template.hpp"
#include"poly.hpp"

using namespace std;

ArgTerm<Template::Fun>& operator+=( ArgTerm<Template::Fun>& x, ArgTerm<Template::Fun> const& y ) {
	auto xf = x.fun().ref<Template::Fun>();
	auto yf = y.fun().ref<Template::Fun>();
	if( xf ) {
		auto const& xe = xf->is_smt();
		if( yf ) {
			auto const& ye = yf->is_smt();
			if( ye ) {
				if( *ye == 0 ) return x;
				if( xe ) {
					if( *xe == 0 ) return x = y;
					return x = *xe + *ye;
				}
			}
			vector<ArgTerm<Template::Fun>> args;
			if( xf->is_fun().contains(Smt::ADD) ) {
				for( auto xarg : x.args() ) {
					args.emplace_back(xarg);
				}
			} else if( xf->is_smt().contains(0) ) {
			} else {
				args.emplace_back(x);
			}
			if( yf->is_fun().contains(Smt::ADD) ) {
				for( auto const& yarg : y.args() ) {
					args.emplace_back(yarg);
				}
			} else {
				args.emplace_back(y);
			}
			return x = app(Smt::ADD,std::move(args));
		}
		if( xe.contains(0) ) {
			return x = y;
		}
	} else if( yf && yf->is_smt().contains(0) ) {
		return x;
	}
	return x = {Smt::ADD,x,y};
}

ArgTerm<Template::Fun>& operator*=( ArgTerm<Template::Fun>& x, ArgTerm<Template::Fun> const& y ) {
	auto xs = x.fun().ref<Template::Fun>();
	auto ys = y.fun().ref<Template::Fun>();
	if( xs ) {
		auto const& xe = xs->is_smt();
		if( xe ) {
			if( *xe == 0 ) return x;
			if( *xe == 1 ) return x = y;
			if( ys ) {
				if( auto const& ye = ys->is_smt() ) {
					return x = *xe * *ye;
				}
			}
		} else if( ys ) {
			auto const& ye = ys->is_smt();
			if( ye ) {
				if( *ye == 0 ) return x = y;
				if( *ye == 1 ) return x;
			}
			vector<ArgTerm<Template::Fun>> args;
			if( xs->is_fun().contains(Smt::MUL) ) {
				for( auto xarg : x.args() ) {
					args.emplace_back(xarg);
				}
			} else {
				args.emplace_back(x);
			}
			if( ys->is_fun().contains(Smt::MUL) ) {
				for( auto const& yarg : y.args() ) {
					args.emplace_back(yarg);
				}
			} else {
				args.emplace_back(y);
			}
			return x = app(Smt::MUL,std::move(args));
		}
	}
	return x = {Smt::MUL,x,y};
}
ArgTerm<Template::Fun> ite(
	ArgTerm<Template::Fun> const& i,
	ArgTerm<Template::Fun> const& t,
	ArgTerm<Template::Fun> const& e
) {
	if( auto const& ifun = i.fun().ref<Template::Fun>() ) {
		if( auto ie = ifun->is_smt() ) {
			if( auto const& ie2 = ie->is_post() ) {
				if( *ie2 == Smt::TRUE ) return t;
				if( *ie2 == Smt::FALSE ) return e;
			}
		}
	}
	return {Smt::ITE,i,t,e};
}
ArgTerm<Template::Fun>& max_eq( ArgTerm<Template::Fun>& x, ArgTerm<Template::Fun> const& y ) {
	auto const& [xsym,xargs] = *x;
	auto const& [ysym,yargs] = *y;
	std::vector<ArgTerm<Template::Fun>> args;
	if( auto const& xf = xsym.ref<Template::Fun>(); xf && xf->is_fun().contains(Smt::MAX) ) {
		for( auto const& xarg : xargs ) {
			args.emplace_back(xarg);
		}
	} else {
		args.emplace_back(x);
	}
	if( auto const& yf = ysym.ref<Template::Fun>(); yf && yf->is_fun().contains(Smt::MAX) ) {
		for( auto const& yarg : yargs ) {
			args.emplace_back(yarg);
		}
	} else {
		args.emplace_back(y);
	}
	return x = app(Smt::MAX,std::move(args));
}

Algebra<Sum<Template::Fun,Arg>,ArgTerm<Template::Fun>> Template::instantiator( Smt::Solver& solver ) {
	return [&]( Sum<Fun,Arg> const& sym, std::vector<ArgTerm<Fun>>&& args )->ArgTerm<Fun>{
		if( auto const& a = sym.ref<Arg>() ) {
			assert( args.empty() );
			return sym;
		}
		auto const& fe = sym.ref<Fun>();
		if( auto const& e = fe->is_smt() ) {
			assert( args.empty() );
			return solver.get_value(*e);
		}
		auto const& f = *fe->is_fun();
		if( f == Smt::ADD ) {
			return chain(ArgTerm<Fun>(Smt::PostExp(0)),BINARY(operator+=),args);
		}
		if( f == Smt::MUL ) {
			return chain(ArgTerm<Fun>(Smt::PostExp(1)),BINARY(operator*=),args);
		}
		if( f == Smt::ITE ) {
			assert( args.size() == 3 );
			return ite(args[0],args[1],args[2]);
		}
		if( f == Smt::MAX ) {
			
		}
		std::vector<ArgTerm<Fun>> targs;
		for( auto&& arg : args ) {
			targs.emplace_back(std::move(arg));
		}
		return app(Fun(f),std::move(targs));
	};
}

static Term<Sum<Template::Fun,Arg>> _deriver_of(
	Exp const& exp,
	Smt::Solver& solver,
	string const& f,
	Trs::Rank const& rank,
	int pos
) {
	auto const& fun = exp.fun();
	size_t n = 0;
	if( fun == "var" ) {
		Opt<Smt::BaseSort> sort;
		Opt<Exp> constrain;
		exp.process_keys(n,[&]( auto const& key, auto const& val ){
			if( key == "sort" ) {
				sort = {Smt::BaseSort::of(val)};
				return true;
			}
			if( key == "constrain" ) {
				constrain = {val};
				return true;
			}
			return false;
		});
		auto const& ret = solver.declare_fresh( sort ? *sort : solver.logic().base_sort() );
		if( constrain ) {
			auto subst = Subst<string>{{"_",ret.exp()}};
			solver.ass(Smt::ALGEBRA(subst(*constrain)));
		}
		return ret;
	}
	if( fun == "arg" ) {
		exp.get_end(n);
		return Arg(pos);
	}
	if( fun == "args" ) {
		auto const& agg = exp.get_arg(n);
		auto const& argexp = exp.get_arg(n);
		exp.get_end(n);
		if( auto const& aggfun = agg.unapplied() ) {
			auto args = vector<Term<Sum<Template::Fun,Arg>>>();
			for( int i = 0; i < rank.arity; i++ ) {
				args.emplace_back(_deriver_of(argexp,solver,f,rank,i));
			}
			return app(*aggfun,std::move(args));
		}
		throw Error("#invalid-arg-aggregator",agg);
	}
	if( fun == "arity" ) {
		while( auto const& arg = exp.gets_arg(n) ) {
			auto const& arity = arg->fun();
			if( arity == "otherwise" || stoi(arity) == rank.arity ) {
				size_t j = 0;
				auto const& aarg = arg->get_arg(j);
				arg->get_end(j);
				return _deriver_of(aarg,solver,f,rank,pos);
			}
		}
		throw Error{"#no-matching-arity",f};
	}
	if( auto const& i = is_int(fun) ) {
		exp.get_end(n);
		return Smt::PostExp(*i);
	}
	auto args = vector<Term<Sum<Template::Fun,Arg>>>();
	while( auto const& arg = exp.gets_arg(n) ) {
		args.emplace_back(_deriver_of(*arg,solver,f,rank,pos));
	}
	exp.get_end(n);
	return app(fun,std::move(args));
}
Deriver<std::string,Template::Fun>
Template::deriver_of( Exp const& e, Trs::Sig const& sig, Smt::Solver& solver ) {
	Map<string,Term<Sum<Template::Fun,Arg>>> map;
	for( auto [f,rank] : sig ) {
		map.emplace(f,_deriver_of(e,solver,f,rank,0));
	}
	return std::move(map);
}
static Exp _posvar = Exp("var",":constrain",Exp(">=","_","0"));
static Exp _1_or_2 = Exp("ite",Exp("var",":sort","Bool"),"2","1");
static Exp _0_or_1 = Exp("ite",Exp("var",":sort","Bool"),"1","0");
Exp const Template::MONO_SUM = Exp{
	Exp("+",Exp("args","+","arg"),_posvar)
};
Exp const Template::MONO_POLY2 = Exp("arity",
	Exp("0",_posvar),
	Exp("1",Exp("+", Exp("*",_1_or_2,"arg"), _posvar)),
	Exp("otherwise",Exp("+",Exp("args","+",Exp("*",_1_or_2,"arg")),_posvar))
);
Exp const Template::SIMP_MAX = Exp("arity",
	Exp("0",_posvar),
	Exp("otherwise",Exp("args","max",Exp("+","arg",_posvar)))
);
Exp const Template::SUM = Exp("arity",
	Exp("0",_posvar),
	Exp("1",Exp("+",Exp("*",_0_or_1,"arg"),_posvar)),
	Exp("otherwise",Exp("+",Exp("args","+",Exp("*",_0_or_1,"arg")),_posvar))
);

void Template::test() {
	cout << "=== Template::test ===" << endl;
	auto z3 = Smt::Z3(Smt::QF_LIA);
	Trs::Sig sig;
	sig.emplace("f",2);
	sig.emplace("g",1);
	sig.emplace("a",0);
	auto der = deriver_of(SUM,sig,z3);
	auto der_intp = der.derive(MPoly::ALGEBRA);
	auto e = Exp("f",Exp("g","x"),"a");
	for( auto [f,rank] : sig ) {
		cout << "der(" << f << ") = " << *der.find(f) << endl;
	}
	auto der_term = der.derive(MPoly::ALGEBRA);
	cout << "der⟦" << "(g x)" << "⟧ = " << der_term(Exp("g","x")) << endl;
	cout << "der⟦a⟧ = " << der_term("a") << endl;
	cout << "der⟦" << e << "⟧ = " << der_term(e) << endl;
	cout << "Poly: " << der_intp(e) << endl;
	cout << "Annotate: " << der_intp.annotate(e) << endl;
	ArgTerm<Template::Fun> x = Smt::PostExp(1);
	x *= "foo";
	cout << x << endl;
}