#include"template.hpp"

using namespace std;

Algebra::Intp<Exp> const Algebra::TERM = {
	[](std::string_view const& f, std::vector<Exp>&& args )->Exp{
		if( args.empty() ) {
			return f;
		}
		Exp ret = f;
		ret.args() = std::move(args);
		return ret;
	}
};

ostream& operator<<( ostream& os, Deriver::Map const& subst ) {
	os << '[' << endl;
	for( auto const& [key,val] : subst ) {
		os << '\t' << key << " := " << val << endl;
	}
	return os << ']';
}

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
	throw Template::Error("#unknown-sort",exp);
}
static Exp derive_inner( string_view const& f, Trs::Rank const& rank, Smt::Solver& solver, Exp const& exp, int pos ) {
	auto const& fun = exp.fun();
	auto const& args = exp.args();
	if( fun == "arg" ) {
		return Exp(":in",pos);
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
					throw Algebra::Error{"#missing-exp",exp};
				}
				Deriver::Map map = {{"_",ret.exp()}};
				solver.ass(Smt::ALGEBRA.eval(map.subst(*it)));
				it++;
			} else {
				throw Algebra::Error{"#malformed",exp};
			}
		}
		return ret.exp();
	}
	if( fun == "args" ) {
		if( args.size() != 2 ) {
			throw Algebra::Error{"#arity-mismatch",exp};
		}
		if( !args[0].args().empty() ) {
			throw Algebra::Error{"#invalid-type",args[0]};
		}
		Exp ret = args[0].fun();
		for( int i = 0; i < rank.arity; i++ ) {
			ret.args().push_back(derive_inner(f,rank,solver,args[1],i));
		}
		return ret;
	}
	if( fun == "arity" ) {
		for( auto const& arg : args ) {
			auto const& arity = arg.fun();
			if( arity == "t" || stoi(arity) == rank.arity ) {
				auto const& aargs = arg.args();
				if( aargs.size() != 1 ) {
					throw Algebra::Error{"#format",fun};
				}
				return derive_inner(f,rank,solver,aargs[0],pos);
			}
		}
		throw Algebra::Error{"#no-matching-arity",f};
	}
	Exp ret = fun;
	for( auto& arg : args ) {
		ret.args().push_back(derive_inner(f,rank,solver,arg,pos));
	}
	return ret;
}

Deriver::Map Template::deriver( Trs::Sig const& sig, Smt::Solver& solver ) const {
	Deriver::Map ret;
	for( auto [f,rank] : sig ) {
		ret.insert(f,derive_inner(f,rank,solver,*this,0));
	}
	return ret;
}

Template const Template::SUM = Exp{
	"arity",
	Exp{"0",Exp{"var","int",":constrain",Exp{">=","_",0}}},
	Exp{"1",
		Exp{"+",
			Exp{"*",Exp{"ite",Exp{"var","bool"},2,1},"arg"},
			Exp{"var","int",":constrain",Exp{">=","_",0}}
		},
	},
	Exp{"t",
		Exp{"+",
			Exp{"args","+",
				Exp{"*",Exp{"ite",Exp{"var","bool"},2,1},"arg"}
			},
			Exp{"var","int",":constrain",Exp{">=","_",0}}
		}
	}
};

