#include"template.hpp"
#include"tpalgebra.hpp"
#include"poly.hpp"

using namespace std;

ArgTerm<Template::Sym>& operator+=( ArgTerm<Template::Sym>& x, ArgTerm<Template::Sym> const& y ) {
	auto xf = x.fun().ref<Template::Sym>();
	auto yf = y.fun().ref<Template::Sym>();
	if( xf ) {
		auto const& xe = xf->is_smt();
		if( yf ) {
			if( auto const& ye = yf->is_smt() ) {
				if( *ye == 0 ) return x;
				if( xe ) {
					if( *xe == 0 ) return x = y;
					return x = *xe + *ye;
				}
			}
			vector<ArgTerm<Template::Sym>> args;
			if( xf->is_fun() && [&](auto const& f){ return f.name == Smt::ADD; } ) {
				for( auto const& xarg : x.args() ) {
					args.emplace_back(xarg);
				}
			} else if( xf->is_smt().contains(0) ) {
			} else {
				args.emplace_back(x);
			}
			if( yf->is_fun() && [&](auto const& f){ return f.name == Smt::ADD; } ) {
				for( auto const& yarg : y.args() ) {
					args.emplace_back(yarg);
				}
			} else {
				args.emplace_back(y);
			}
			if( args.size() == 1 ) return x = args[0];
			return x = app(Template::Fun(Smt::ADD),std::move(args));
		}
		if( xe.contains(0) ) {
			return x = y;
		}
	} else if( yf ) {
		if( yf->is_smt().contains(0) ) {
			return x;
		}
		if( yf->is_fun() && [&](auto const& f){ return f.name == Smt::ADD; } ) {
			vector<ArgTerm<Template::Sym>> args = {x};
			for( auto const& yarg : y.args() ) {
				args.emplace_back(yarg);
			}
			if( args.size() == 1 ) return x = args[0];
			return x = app(Template::Fun(Smt::ADD),std::move(args));
		}
	}
	return x = {Template::Fun(Smt::ADD),x,y};
}

ArgTerm<Template::Sym>& operator*=( ArgTerm<Template::Sym>& x, ArgTerm<Template::Sym> const& y ) {
	auto xs = x.fun().ref<Template::Sym>();
	auto ys = y.fun().ref<Template::Sym>();
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
			vector<ArgTerm<Template::Sym>> args;
			if( xs->is_fun() && [&](auto const& f){ return f.name == Smt::MUL; } ) {
				for( auto xarg : x.args() ) {
					args.emplace_back(xarg);
				}
			} else {
				args.emplace_back(x);
			}
			if( ys->is_fun() && [&](auto const& f){ return f.name == Smt::MUL; } ) {
				for( auto const& yarg : y.args() ) {
					args.emplace_back(yarg);
				}
			} else {
				args.emplace_back(y);
			}
			return x = app(Template::Fun(Smt::MUL),std::move(args));
		}
	}
	return x = {Template::Fun(Smt::MUL),x,y};
}
ArgTerm<Template::Sym> ite(
	ArgTerm<Template::Sym> const& i,
	ArgTerm<Template::Sym> const& t,
	ArgTerm<Template::Sym> const& e
) {
	if( auto const& ifun = i.fun().ref<Template::Sym>() ) {
		if( auto ie = ifun->is_smt() ) {
			if( auto const& ie2 = ie->is_post() ) {
				if( *ie2 == Smt::TRUE ) return t;
				if( *ie2 == Smt::FALSE ) return e;
			}
		}
	}
	return {Template::Fun(Smt::ITE),i,t,e};
}
ArgTerm<Template::Sym>& max_eq( ArgTerm<Template::Sym>& x, ArgTerm<Template::Sym> const& y ) {
	auto const& [xsym,xargs] = *x;
	auto const& [ysym,yargs] = *y;
	std::vector<ArgTerm<Template::Sym>> args;
	if( xsym.ref<Template::Sym>() && [&]( auto const& sym ){
		return sym.is_fun() && [&]( auto const& f ){ return f.name == Smt::MAX; };
	} ) {
		for( auto const& xarg : xargs ) {
			args.emplace_back(xarg);
		}
	} else {
		args.emplace_back(x);
	}
	if( ysym.ref<Template::Sym>() && [&]( auto const& sym ){
		return sym.is_fun() && [&]( auto const& f ){ return f.name == Smt::MAX; };
	} ) {
		for( auto const& yarg : yargs ) {
			args.emplace_back(yarg);
		}
	} else {
		args.emplace_back(y);
	}
	return x = app(Template::Fun(Smt::MAX),std::move(args));
}

Algebra<Sum<Template::Sym,Arg>,ArgTerm<Template::Sym>> Template::instantiator( Smt::Solver& solver ) {
	return [&]( Sum<Sym,Arg> const& sym, std::vector<ArgTerm<Sym>>&& args )->ArgTerm<Sym>{
		if( auto const& a = sym.ref<Arg>() ) {
			assert( args.empty() );
			return sym;
		}
		auto const& fe = sym.ref<Sym>();
		if( auto const& e = fe->is_smt() ) {
			assert( args.empty() );
			return solver.get_value(*e);
		}
		if( auto const& f = fe->is_fun() ) {
			if( f->name == Smt::ADD ) {
				return chain(ArgTerm<Sym>(Smt::PostExp(0)),BINARY(operator+=),args);
			}
			if( f->name == Smt::MUL ) {
				return chain(ArgTerm<Sym>(Smt::PostExp(1)),BINARY(operator*=),args);
			}
			if( f->name == Smt::ITE ) {
				assert( args.size() == 3 );
				return ite(args[0],args[1],args[2]);
			}
			if( f->name == Smt::MAX ) {
				
			}
		}
		std::vector<ArgTerm<Sym>> targs;
		for( auto&& arg : args ) {
			targs.emplace_back(std::move(arg));
		}
		return app(*fe,std::move(targs));
	};
}

Term<Sum<Template::Sym,Arg>> Template::Deriver::_deriver_of(
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
			auto args = vector<Term<Sum<Template::Sym,Arg>>>();
			for( int i = 0; i < rank.arity; i++ ) {
				args.emplace_back(_deriver_of(argexp,f,rank,i,finfo));
			}
			return app(Template::Fun(*aggfun),std::move(args));
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
	auto args = vector<Term<Sum<Template::Sym,Arg>>>();
	while( auto const& arg = exp.gets_arg(n) ) {
		args.emplace_back(_deriver_of(*arg,f,rank,pos,finfo));
	}
	exp.get_end(n);
	if( fun == Smt::ADD ) {
		Term<Sum<Sym,Arg>> ret = Smt::PostExp(0);
		for( auto&& arg : args ) {
			ret += std::move(arg);
		}
		return ret;
	}
	return app(Template::Fun(fun),std::move(args));
}
void Template::Deriver::extend_sig( std::string const& f, Trs::Rank const& rank ) & {
	if( sig.find(f) ) return;
	auto [finfo,fl] = sig.emplace(
		f, FunInfo{
			.triv = false//_solver.declare_const("t_"+escape(f),Smt::BOOL),
		}
	);
	if( log & INIT ) cerr << "; intp triv[" << f << "] := " << finfo.triv << endl;
	for( size_t i = 0; i < rank.arity; i++ ) {
		auto const& infl = _solver.declare_const("i_"+escape(f)+"_"+to_string(i), Smt::BOOL);
		auto const& used = _solver.declare_const("u_"+escape(f)+"_"+to_string(i),Smt::BOOL);
		auto const& arg = finfo.args.emplace_back(ArgInfo{
			.infl = infl,
			.used = used,
		});
	}
	if( log & INIT ) {
		cerr << "; intp infl[" << f << "] := (" << print_list( 0, rank.arity, [&](size_t i){ return finfo.args[i].infl; } ) << ")" << endl;
		cerr << "; intp used[" << f << "] := (" << print_list( 0, rank.arity, [&](size_t i){ return finfo.args[i].used; } ) << ")" << endl;
	}
	assign(f,_deriver_of(_template_exp,f,rank,0,finfo));
} 

static Exp const _POSCONST = Exp("var",":constrain",
	Exp("and",
		Exp(">=","_","0"),
		Exp("=>","#triv",Exp("=","_","0"))// trivial requires 0 variable
	)
);
static Exp _bool_constrain( Exp const& c ) {
	return Exp("var",":sort","Bool",":constrain",c);
}
static Exp const _1_OR_2 = Exp("ite",Exp("var",":sort","Bool"),"2","1");
Exp _0_or_1_constrain( Exp const& c ) {
	return Exp("ite",Exp("var",":sort","Bool",":constrain",c),"1","0");
}
static Exp const _MONO = Exp("=>","#mono","_");// monotonicity requires non-zero coefficient
static Exp const _USED = Exp("=>","_","#used");// non-zero coefficient implies used
static Exp const _INFL = Exp("=>","#infl","_");// inflationary position requires non-zero coefficient (and more)

Exp const Template::MONO_SUM = Exp{
	Exp("+",Exp("args","+","arg"),_POSCONST)
};
Exp const Template::MONO_POLY2 = Exp("arity",
	Exp("0",_POSCONST),
	Exp("1",Exp("+", Exp("*",_1_OR_2,"arg"), _POSCONST)),
	Exp("otherwise",Exp("+",Exp("args","+",Exp("*",_1_OR_2,"arg")),_POSCONST))
);
static Exp const _SUMCOEFF = _0_or_1_constrain(Exp("and",_MONO,_INFL,_USED));
Exp const Template::SUM = Exp("arity",
	Exp("0",_POSCONST),
	Exp("1",Exp("+",Exp("*",_SUMCOEFF,"arg"),_POSCONST)),
	Exp("otherwise",Exp("+",Exp("args","+",Exp("*",_SUMCOEFF,"arg")),_POSCONST))
);
Exp const Template::SIMP_MAX = Exp("arity",
	Exp("0",_POSCONST),
	Exp("otherwise",Exp("args","max",Exp("+","arg",_POSCONST)))
);
Exp const Template::MAX = Exp("arity",
	Exp("0",_POSCONST),
	Exp("1",Exp("+",Exp("ite",_bool_constrain(Exp("and",_USED,_INFL)),"arg","0"),_POSCONST)),
	Exp("otherwise",Exp("args","max",Exp("ite",_bool_constrain(Exp("and",_USED,_INFL)),Exp("+","arg",_POSCONST),"0")))
);
Exp const Template::MAT2N = Exp("tp",
	Exp("+",Exp("args","+",
		Exp("+",
			Exp("*",_POSCONST,Exp("prj0","arg")),
			Exp("*",_POSCONST,Exp("prj1","arg"))
		)),
		_POSCONST
	),
	Exp("+",Exp("args","+",
		Exp("+",
			Exp("*",_POSCONST,Exp("prj0","arg")),
			Exp("*",_POSCONST,Exp("prj1","arg"))
		)),
		_POSCONST
	)
);
Exp const Template::MAT2B = Exp("tp",
	Exp("+",Exp("args","+",
		Exp("+",
			Exp("ite",_bool_constrain(Exp("and",_MONO,_USED,_INFL)),Exp("prj0","arg"),"0"),
			Exp("ite",_bool_constrain(Exp("and",_USED)),Exp("prj1","arg"),"0")
		)),
		_POSCONST
	),
	Exp("+",Exp("args","+",
		Exp("+",
			Exp("ite",_bool_constrain(_USED),Exp("prj0","arg"),"0"),
			Exp("ite",_bool_constrain(Exp("and",_USED,_INFL)),Exp("prj1","arg"),"0")
		)),
		_POSCONST
	)
);

void Template::test() {
	cout << "=== Template::test ===" << endl;
	auto z3 = Smt::Solver::of(Exp("z3","LIA",":tee","cout"));
	Trs::Sig sig;
	sig.emplace("f",2);
	sig.emplace("g",1);
	sig.emplace("a",0);
	auto der = Template::Deriver(SUM,z3,false,INIT&DEBUG);
	der.extend_sig(sig);
	auto der_intp = der.derive(Poly::ALGEBRA);
	auto e = Exp("f",Exp("g","x"),"a");
	cout << der << endl;
	auto der_term = der.derive(Poly::ALGEBRA);
	cout << "der⟦" << "(g x)" << "⟧ = " << der_term(Exp("g","x")) << endl;
	cout << "der⟦a⟧ = " << der_term("a") << endl;
	cout << "der⟦" << e << "⟧ = " << der_term(e) << endl;
	cout << "Poly: " << der_intp(e) << endl;
	cout << "Annotate: " << der_intp.annotate(e) << endl;
	ArgTerm<Template::Sym> x = Smt::PostExp(1);
	x *= "foo";
	cout << x << endl;
	auto mat2b = Template::Deriver(MAT2B,z3,false,INIT&DEBUG);
	mat2b.extend_sig(sig);
	auto mat_intp = mat2b.derive(tuple_algebra<Poly::Range,Poly>({Poly::POS,Poly::POS}));
	auto gx = Exp("g","x");
	cout << "mat⟦" << gx << "⟧ = " << mat_intp(gx) << endl;
	auto fxy = Exp("f","x","y");
	auto mfxy = mat_intp(fxy);
	cout << "mat⟦" << fxy << "⟧ = " << mfxy << endl;
	cout << "(<=? " << fxy << " x) : " << order(mfxy,mat_intp(Exp("x")),z3) << endl;
}