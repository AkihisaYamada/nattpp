#include"poly.hpp"

using namespace std;


static void arity_check( bool test, Exp const& exp ) {
	if( !test ) {
		throw Algebra::Error{"#arity-mismatch",exp};
	}
}


static Opt<Poly::Sig> poly_fun( string const& str ) {
	if( str == "+" ) {
		return Poly::Sig(in_place_type<Poly::Add>);
	}
	if( str == "*" ) {
		return Poly::Sig(in_place_type<Poly::Mul>);
	}
	return {};
}

Term<Sum<Poly::Sig,Algebra::Arg>> Poly::Template::_deriver_inner(
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
		return Algebra::Arg(pos);
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
			Subst<string> subst = {{"_",ret.exp()}};
			solver.ass(Smt::ALGEBRA.eval(subst.eval(*constrain)));
		}
		return ret;
	}
	if( fun == "args" ) {
		auto const& agg = exp.get_arg(n);
		auto const& argexp = exp.get_arg(n);
		exp.get_end(n);
		if( auto aggfun = agg.unapplied() )
		if( auto pfun = poly_fun(*aggfun) ) {
			auto ret = Term<Sum<Sig,Algebra::Arg>>(*pfun);
			for( int i = 0; i < rank.arity; i++ ) {
				ret.args().push_back(_deriver_inner(f,rank,solver,argexp,i));
			}
			return ret;
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
		auto iexp = exp.get_arg(n);
		auto texp = exp.get_arg(n);
		auto eexp = exp.get_arg(n);
		exp.get_end(n);
		auto i = _deriver_inner(f,rank,solver,iexp,pos);
		if( auto ifun = i.fun().ref<Sig>() )
		if( auto ie = ifun->ref<Smt::PreExp>() ) {
			return Term<Sum<Sig,Algebra::Arg>>(
				Cond{*ie},
				_deriver_inner(f,rank,solver,texp,pos),
				_deriver_inner(f,rank,solver,eexp,pos)
			);
		}
		throw Error{"#template-format",exp};
	}
	if( auto pfun = poly_fun(fun) ) {
		auto ret = Term<Sum<Sig,Algebra::Arg>>(*pfun);
		while( auto const& arg = exp.gets_arg(n) ) {
			ret.args().push_back(_deriver_inner(f,rank,solver,*arg,pos));
		}
		exp.get_end(n);
		return ret;
	}
	if( auto i = to_int(fun) ) {
		exp.get_end(n);
		return Smt::PostExp(*i);
	}
	throw Error{"#template-format",exp};
}

Algebra::Deriver<string,Poly::Sig> Poly::Template::deriver( Trs::Sig const& sig, Smt::Solver& solver ) const {
	Map<string,Term<Sum<Sig,Algebra::Arg>>> map;
	for( auto [f,rank] : sig ) {
		map.insert(f,_deriver_inner(f,rank,solver,*this,0));
	}
	return [map = move(map)]( string const& f )->Term<Sum<Sig,Algebra::Arg>> {
		if( auto val = map.find(f) ) {
			return *val;
		}
		return Var(f,POS);
	};
}

Poly::Template const Poly::Template::MONO_SUM = Exp{
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

Poly::Template const Poly::Template::SUM = Exp{
	"arity",
	Exp{"0",Exp{"var",":constrain",Exp{">=","_","0"}}},
	Exp{"1",
		Exp{"+",
			Exp{"*",Exp{"ite",Exp{"var",":sort","bool"},"1","0"},"arg"},
			Exp{"var",":constrain",Exp{">=","_","0"}}
		},
	},
	Exp{"otherwise",
		Exp{"+",
			Exp{"args","+",
				Exp{"*",Exp{"ite",Exp{"var",":sort","bool"},"1","0"},"arg"}
			},
			Exp{"var",":constrain",Exp{">=","_","0"}}
		}
	}
};

