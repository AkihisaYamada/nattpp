#include "term.hpp"

using namespace std;

ostream& operator<<( ostream& os, TRS::Rule const& rule ) {
	return os << rule.first << " -> " << rule.second;
}

ostream& operator<<( ostream& os, TRS::Rules const& system ) {
	for( auto const& rule : system ) {
		os << '\t' << rule << endl;
	}
	return os;
}

Opt<TRS::Exp> TRS::Reader::reads_term() {
	if( auto sym = reads_sym() ) {
		if( auto info = _sig.find(*sym) ) {
			if( info->arity() != 0 ) {
				throw Error({"#unapplied-fun",*sym});
			}
			return {*sym};
		}
		return *sym;
	}
	if( opens() ) {
		auto const& fun = reads_sym();
		if( !fun ) {
			throw Error("#nil");
		}
		auto const& info = _sig.find(*fun);
		if( !info ) {
			throw Error{"#applied-var",*fun};
		}
		unsigned char arity = info->arity();
		if( arity == 0 ) {
			throw Error{"#applied-const",*fun};
		}
		vector<::Exp> args;
		for( unsigned char n = 0; n < arity; n++ ) {
			auto const& arg = reads_term();
			if( !arg ) {
				throw Error{"#too-few-args",{*fun,std::move(args)}};
			}
			args.push_back(*arg);
		}
		if( !closes() ) {
			throw Error{"too-many-args",{*fun,std::move(args)}};
		}
		return Exp{*fun,std::move(args)};
	}
	return {};
}