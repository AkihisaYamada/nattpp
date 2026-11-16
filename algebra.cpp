#include"algebra.hpp"

ArgTerm<std::string> argterm_of( Exp const& exp ) {
	if( exp.fun() == "#arg" ) {
		auto proc = exp.proc();
		ArgTerm<std::string> ret = Arg(proc.get_arg().as_nat());
		proc, Exp::END;
		return ret;
	}
	std::vector<ArgTerm<std::string>> args;
	for( auto const& arg : exp.args() ) {
		args.emplace_back(argterm_of(arg));
	}
	return app(exp.fun(),std::move(args));
}

StrDeriver StrDeriver::of( Exp const& exp ) {
	StrDeriver ret;
	if( exp.fun() == "subst" ) {
		uint n = 0;
		while( auto p = exp.gets_arg(n) ) {
			auto [x,args] = **p;
			if( args.size() != 1 ) throw Error("#syntax");
			ret.assign(x,argterm_of(args[0]));
		}
	}
	return ret;
}
