#include "term.hpp"

using namespace std;

ostream& operator<<( ostream& os, Term const& term ) {
	if( auto var = term.var() ) {
		return os << var;
	} else if( auto app = term.app() ) {
		os << app->first;
		auto const& args = app->second;
		auto it = args.begin();
		if( it != args.end() ) {
			os << '(' << *it;
			for(;;) {
				it++;
				if( it == args.end() ) break;
				os << ',' << *it;
			}
			os << ')';
		}
		return os;
	} else {
		assert(false);
	}
}

optional<Term> Term::Reader::reads_term() {
	if( auto sym = reads_sym() ) {
		if( auto info = _sig.find(*sym) ) {
			if( info->arity() != 0 ) {
				throw Error(Term("#unapplied-fun",{*sym}));
			}
			return Term(*sym);
		}
		return Var(*sym);
	}
	if( opens() ) {
		auto const& fun = reads_sym();
		if( !fun ) {
			throw Error(Term("#nil"));
		}
		auto const& info = _sig.find(*fun);
		if( !info ) {
			throw Error(Term("#applied-var",{*fun}));
		}
		unsigned char arity = info->arity();
		if( arity == 0 ) {
			throw Error(Term("#applied-const",{*fun}));
		}
		vector<Term> args;
		for( unsigned char n = 0; n < arity; n++ ) {
			auto const& arg = reads_term();
			if( !arg ) {
				throw Error(Term("#too-few-args",{Term(*fun,std::move(args))}));
			}
			args.push_back(*arg);
		}
		if( !closes() ) {
			throw Error(Term("too-many-args",{Term(*fun,std::move(args))}));
		}
		return Term(*fun,std::move(args));
	}
	return nullopt;
}