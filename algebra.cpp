#include"poly.hpp"

using namespace std;


static void arity_check( bool test, Exp const& exp ) {
	if( !test ) {
		throw Algebra::Error{"#arity-mismatch",exp};
	}
}

static Smt::BaseSort base_sort_of( Exp const& exp ) {
	if( exp == "int" ) {
		return Smt::INT;
	}
	if( exp == "bool" ) {
		return Smt::BOOL;
	}
	throw Poly::Error("#unknown-sort",exp);
}

static Opt<int> safe_stoi( string const& str ) try {
	return stoi(str);
} catch( exception const& err ) {
	return {};
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

Tree<Sum<Poly::Sig,Algebra::Arg>> Poly::Template::_deriver_inner( string const& f, Trs::Rank const& rank, Smt::Solver& solver, Exp const& exp, int pos ) {
	auto const& fun = exp.fun();
	auto const& args = exp.args();
	if( fun == "arg" ) {
		return Algebra::Arg(pos);
	}
	if( fun == "var" ) {
		auto it = args.begin();
		arity_check( it != args.end(), exp );
		Smt::BaseSort base = base_sort_of(*it);
		arity_check( it->args().empty(), *it );
		it++;
		auto const& ret = solver.declare_fresh(base);
		while( it != args.end() ) {
			if( *it == ":constrain" ) {
				it++;
				if( it == args.end() ) {
					throw Error{"#missing-exp",exp};
				}
				Subst<string> subst = {{"_",ret.exp()}};
				solver.ass(Smt::ALGEBRA.eval(subst.eval(*it)));
				it++;
			} else {
				throw Error{"#malformed",exp};
			}
		}
		return ret;
	}
	if( fun == "args" ) {
		if( args.size() != 2 ) {
			throw Error{"#arity-mismatch",exp};
		}
		if( !args[0].args().empty() ) {
			throw Error{"#invalid",exp};
		}
		auto& afun = args[0].fun();
		auto pfun = poly_fun(afun);
		if( !pfun ) {
			throw Error{"#invalid",exp};
		}
		auto ret = Tree<Sum<Sig,Algebra::Arg>>(*pfun);
		for( int i = 0; i < rank.arity; i++ ) {
			ret.args().push_back(_deriver_inner(f,rank,solver,args[1],i));
		}
		return ret;
	}
	if( fun == "arity" ) {
		for( auto const& arg : args ) {
			auto const& arity = arg.fun();
			if( arity == "t" || stoi(arity) == rank.arity ) {
				auto const& aargs = arg.args();
				if( aargs.size() != 1 ) {
					throw Error{"#format",fun};
				}
				return _deriver_inner(f,rank,solver,aargs[0],pos);
			}
		}
		throw Error{"#no-matching-arity",f};
	}
	if( auto pfun = poly_fun(fun) ) {
		auto ret = Tree<Sum<Sig,Algebra::Arg>>(*pfun);
		for( auto& arg : args ) {
			ret.args().push_back(_deriver_inner(f,rank,solver,arg,pos));
		}
		return ret;
	}
	if( fun == "ite" ) {
		if( args.size() != 3 ) {
			throw Error{"#arity-mismatch",exp};
		}
		auto it = _deriver_inner(f,rank,solver,args[0],pos);
		auto tt = _deriver_inner(f,rank,solver,args[1],pos);
		auto et = _deriver_inner(f,rank,solver,args[2],pos);
		try {
			auto i = *it.fun().ref<Sig>()->ref<Smt::PostExp>();
			auto t = *tt.fun().ref<Sig>()->ref<Smt::PostExp>();
			auto e = *et.fun().ref<Sig>()->ref<Smt::PostExp>();
			return Smt::ite(i,t,e);
		} catch( exception e ) {
			throw Error{"#template-format",exp};
		}
	}
	if( auto i = safe_stoi(fun) ) {
		return Smt::PostExp(*i);
	}
	throw Error{"#template-format",exp};
}

Algebra::Deriver<string,Poly::Sig> Poly::Template::deriver( Trs::Sig const& sig, Smt::Solver& solver ) const {
	Map<string,Tree<Sum<Sig,Algebra::Arg>>> map;
	for( auto [f,rank] : sig ) {
		map.insert(f,_deriver_inner(f,rank,solver,*this,0));
	}
	return [map = move(map)]( string const& f )->Tree<Sum<Sig,Algebra::Arg>> {
		if( auto val = map.find(f) ) {
			return *val;
		}
		return Var(f,POS);
	};
}

Poly::Template const Poly::Template::SUM = Exp{
	"arity",
	Exp{"0",Exp{"var","int",":constrain",Exp{">=","_","0"}}},
	Exp{"1",
		Exp{"+",
			Exp{"*",Exp{"ite",Exp{"var","bool"},"2","1"},"arg"},
			Exp{"var","int",":constrain",Exp{">=","_","0"}}
		},
	},
	Exp{"t",
		Exp{"+",
			Exp{"args","+",
				Exp{"*",Exp{"ite",Exp{"var","bool"},"2","1"},"arg"}
			},
			Exp{"var","int",":constrain",Exp{">=","_","0"}}
		}
	}
};

