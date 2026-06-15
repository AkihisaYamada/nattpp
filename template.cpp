#include"poly.hpp"

using namespace std;

static Opt<Poly::Sig> is_poly_fun( string const& str ) {
	if( str == "+" ) {
		return Poly::Sig(in_place_type<Poly::Add>);
	}
	if( str == "*" ) {
		return Poly::Sig(in_place_type<Poly::Mul>);
	}
	return {};
}

Term<Sum<Poly::Sig,Arg>> Poly::_deriver_inner(
	string const& f,
	Trs::Rank const& rank,
	Smt::Solver& solver,
	Exp const& exp,
	int pos
) {
	auto const& fun = exp.fun();
	size_t n = 0;
	if( fun == "arg" ) {
		exp.get_end(n);
		return Arg(pos);
	}
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
	if( fun == "args" ) {
		auto const& agg = exp.get_arg(n);
		auto const& argexp = exp.get_arg(n);
		exp.get_end(n);
		if( auto const& aggfun = agg.unapplied() )
			if( auto const& pfun = is_poly_fun(*aggfun) ) {
				auto args = vector<Term<Sum<Sig,Arg>>>();
				for( int i = 0; i < rank.arity; i++ ) {
					args.emplace_back(_deriver_inner(f,rank,solver,argexp,i));
				}
				return app(*pfun,std::move(args));
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
				return _deriver_inner(f,rank,solver,aarg,pos);
			}
		}
		throw Error{"#no-matching-arity",f};
	}
	if( fun == "ite" ) {
		auto const& iexp = exp.get_arg(n);
		auto const& texp = exp.get_arg(n);
		auto const& eexp = exp.get_arg(n);
		exp.get_end(n);
		auto i = _deriver_inner(f,rank,solver,iexp,pos);
		if( auto const& ifun = i.fun().ref<Sig>() )
			if( auto const& ie = ifun->ref<Smt::PreExp>() ) {
				return Term<Sum<Sig,Arg>>(
					Cond{*ie},
					_deriver_inner(f,rank,solver,texp,pos),
					_deriver_inner(f,rank,solver,eexp,pos)
				);
			}
		throw Error{"#template-format",exp};
	}
	if( auto const& pfun = is_poly_fun(fun) ) {
		auto args = vector<Term<Sum<Sig,Arg>>>();
		while( auto const& arg = exp.gets_arg(n) ) {
			args.emplace_back(_deriver_inner(f,rank,solver,*arg,pos));
		}
		exp.get_end(n);
		return app(*pfun,std::move(args));
	}
	if( auto const& i = is_int(fun) ) {
		exp.get_end(n);
		return Smt::PostExp(*i);
	}
	throw Error{"#template-format",exp};
}

Deriver<string,Poly::Sig> Poly::deriver( Exp const& e, Trs::Sig const& sig, Smt::Solver& solver ) {
	Map<string,Term<Sum<Sig,Arg>>> map;
	for( auto [f,rank] : sig ) {
		map.emplace(f,_deriver_inner(f,rank,solver,e,0));
	}
	return [map = std::move(map)]( string const& f )->Term<Sum<Sig,Arg>> {
		if( auto val = map.find(f) ) {
			return *val;
		}
		return Var(f,POS);
	};
}

Exp const Poly::MONO_SUM = Exp{
	Exp{"+",Exp{"args","+","arg"},Exp{"var",":constrain",Exp{">=","_","0"}}}
};

Exp const Poly::MONO_POLY2 = Exp{
	"arity",
	Exp{"0",Exp{"var",":constrain",Exp{">=","_","0"}}},
	Exp{"1",
		Exp{"+",
			Exp{"*",Exp{"ite",Exp{"var",":sort","Bool"},"2","1"},"arg"},
			Exp{"var",":constrain",Exp{">=","_","0"}}
		},
	},
	Exp{"otherwise",
		Exp{"+",
			Exp{"args","+",
				Exp{"*",Exp{"ite",Exp{"var",":sort","Bool"},"2","1"},"arg"}
			},
			Exp{"var",":constrain",Exp{">=","_","0"}}
		}
	}
};

Exp const Poly::SUM = Exp{
	"arity",
	Exp{"0",Exp{"var",":constrain",Exp{">=","_","0"}}},
	Exp{"1",
		Exp{"+",
			Exp{"*",Exp{"ite",Exp{"var",":sort","Bool"},"1","0"},"arg"},
			Exp{"var",":constrain",Exp{">=","_","0"}}
		},
	},
	Exp{"otherwise",
		Exp{"+",
			Exp{"args","+",
				Exp{"*",Exp{"ite",Exp{"var",":sort","Bool"},"1","0"},"arg"}
			},
			Exp{"var",":constrain",Exp{">=","_","0"}}
		}
	}
};

