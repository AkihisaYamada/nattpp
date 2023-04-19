#include"algebra.hpp"

using namespace std;

const Algebra::Intp<Exp> Algebra::TERM = {
	[](std::string_view const& f, std::vector<Exp>&& args ){
		return Exp(f,std::move(args));
	}
};

ostream& operator<<( ostream& os, Deriver::Map const& subst ) {
	os << '[' << endl;
	for( auto const& [key,val] : subst ) {
		os << '\t' << key << " := " << val << endl;
	}
	return os << ']';
}

static Exp derive_inner( string_view const& f, Trs::Rank const& rank, Smt::Solver& solver, Exp const& exp, int pos ) {
	if( auto sym = exp.sym() ) {
		if( *sym == "arg" ) {
			return {":in",pos};
		}
		return exp;
	}
	if( auto app = exp.app() ) {
		auto const& [fun,args] = *app;
		if( fun == "var" ) {
			if( args.size() != 1 ) {
				throw Exp::Error{"#arity-mismatch",exp};
			}
			auto sort = args[0].sym();
			if( !sort ) {
				throw Exp::Error{"#expects","symbol",exp};
			}
			return solver.declare_fresh(*sort);
		}
		if( fun == "args" ) {
			if( args.size() != 2 ) {
				throw Exp::Error{"#arity-mismatch",exp};
			}
			auto vfun = args[0].sym();
			if( !vfun ) {
				throw Exp::Error{"#expects","symbol",exp};
			}
			vector<Exp> vargs;
			for( int i = 0; i < rank.arity; i++ ) {
				vargs.push_back(derive_inner(f,rank,solver,args[1],i));
			}
			return Exp(*vfun,std::move(vargs));
		}
		vector<Exp> vargs;
		for( auto& arg : args ) {
			vargs.push_back(derive_inner(f,rank,solver,arg,pos));
		}
		return Exp(fun,std::move(vargs));
	}
	assert(false);
}
Deriver::Map Deriver::Template::deriver( Trs::Sig const& sig, Smt::Solver& solver ) {
	Deriver::Map ret;
	for( auto [f,rank] : sig ) {
		ret.insert(f,derive_inner(f,rank,solver,_exp,0));
	}
	return ret;
}


