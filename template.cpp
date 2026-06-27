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

Term<Sum<Template::Fun,Arg>> Template::Deriver::_deriver_of(
	Exp const& exp,
	string const& f,
	Trs::Rank const& rank,
	int pos,
	FunInfo const& finfo
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
		auto const& ret = _solver.declare_fresh( sort ? *sort : _solver.logic().base_sort() );
		if( constrain ) {
			_solver.ass(
				Smt::ALGEBRA.extend({
					{"_",[&](auto){ return ret; }},
					{"#mono",[&](auto){ return mono; }},
					{"#infl",[&](auto){ return finfo.args[pos].infl; }},
					{"#used",[&](auto){ return finfo.args[pos].used; }},
					{"#triv",[&](auto){ return finfo.triv; }},
				})(*constrain)
			);
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
				args.emplace_back(_deriver_of(argexp,f,rank,i,finfo));
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
				return _deriver_of(aarg,f,rank,pos,finfo);
			}
		}
		throw Error{"#no-matching-arity",f};
	}
	if( fun == "#mono" ) {
		return mono;
	}
	if( fun == "#infl" ) {
		return finfo.args[pos].infl;
	}
	if( fun == "#used" ) {
		return finfo.args[pos].used;
	}
	if( fun == "#triv" ) {
		return finfo.triv;
	}
	if( auto const& i = is_int(fun) ) {
		exp.get_end(n);
		return Smt::PostExp(*i);
	}
	auto args = vector<Term<Sum<Template::Fun,Arg>>>();
	while( auto const& arg = exp.gets_arg(n) ) {
		args.emplace_back(_deriver_of(*arg,f,rank,pos,finfo));
	}
	exp.get_end(n);
	return app(fun,std::move(args));
}
void Template::Deriver::extend_sig( std::string const& f, Trs::Rank const& rank ) & {
	auto [finfo,fl] = sig.emplace(
		f, FunInfo{
			.triv = false//_solver.declare_fresh(Smt::BOOL),
				//_solver.declare_const("t"+escape(f),Smt::BOOL),
		}
	);
	if( log & INIT ) cerr << "; intp triv[" << f << "] := " << finfo.triv << endl;
	for( size_t i = 0; i < rank.arity; i++ ) {
		auto const& infl = _solver.declare_const("i"+escape(f)+"_"+to_string(i), Smt::BOOL);
//		auto const& used = _solver.declare_const("u"+escape(f)+"_"+to_string(i),Smt::BOOL);
		auto const& arg = finfo.args.emplace_back(ArgInfo{
			.infl = infl,
			.used = infl,
		});
	}
	if( log & INIT ) {
		cerr << "; intp infl[" << f << "] := (" << print_list( 0, rank.arity, [&](size_t i){ return finfo.args[i].infl; } ) << ")" << endl;
		cerr << "; intp used[" << f << "] := (" << print_list( 0, rank.arity, [&](size_t i){ return finfo.args[i].used; } ) << ")" << endl;
	}
	assign(f,_deriver_of(_template_exp,f,rank,0,finfo));
} 

static Exp const _POSVAR = Exp("var",":constrain",
	Exp("and",
		Exp(">=","_","0"),
		Exp("=>","#triv",Exp("=","_","0"))// trivial requires 0 variable
	)
);
static Exp const _1_OR_2 = Exp("ite",Exp("var",":sort","Bool"),"2","1");
static Exp const _0_OR_1 = Exp("ite",Exp("var",":sort","Bool"),"1","0");
Exp _0_or_1_constrain( Exp const& c ) {
	return Exp("ite",Exp("var",":sort","Bool",":constrain",c),"1","0");
}
static Exp const _MONO = Exp("=>","#mono","_");// monotonicity requires non-zero coefficient
static Exp const _USED = Exp("=>","_","#used");// non-zero coefficient implies used
static Exp const _INFL = Exp("=>","#infl","_");// inflationary position requires non-zero coefficient (and more)

Exp const Template::MONO_SUM = Exp{
	Exp("+",Exp("args","+","arg"),_POSVAR)
};
Exp const Template::MONO_POLY2 = Exp("arity",
	Exp("0",_POSVAR),
	Exp("1",Exp("+", Exp("*",_1_OR_2,"arg"), _POSVAR)),
	Exp("otherwise",Exp("+",Exp("args","+",Exp("*",_1_OR_2,"arg")),_POSVAR))
);
static Exp const _SUMCOEFF = _0_or_1_constrain(Exp("and",_MONO,_INFL,_USED));
Exp const Template::SUM = Exp("arity",
	Exp("0",_POSVAR),
	Exp("1",Exp("+",Exp("*",_SUMCOEFF,"arg"),_POSVAR)),
	Exp("otherwise",Exp("+",Exp("args","+",Exp("*",_SUMCOEFF,"arg")),_POSVAR))
);
Exp const Template::SIMP_MAX = Exp("arity",
	Exp("0",_POSVAR),
	Exp("otherwise",Exp("args","max",Exp("+","arg",_POSVAR)))
);
Exp const Template::MAX = Exp("arity",
	Exp("0",_POSVAR),
	Exp("1",Exp("+",Exp("ite","#infl","arg","0"),_POSVAR)),
	Exp("otherwise",Exp("args","max",Exp("ite","#infl",Exp("+","arg",_POSVAR),"0")))
);

void Template::test() {
	cout << "=== Template::test ===" << endl;
	auto z3 = Smt::Z3(Smt::QF_LIA);
	Trs::Sig sig;
	sig.emplace("f",2);
	sig.emplace("g",1);
	sig.emplace("a",0);
	auto der = Template::Deriver(SUM,z3,INIT&DEBUG);
	der.extend_sig(sig);
	auto der_intp = der.derive(MPoly::ALGEBRA);
	auto e = Exp("f",Exp("g","x"),"a");
	cout << der << endl;
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