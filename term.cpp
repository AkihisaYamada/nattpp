#include "term.hpp"

using namespace std;

Exp const Sig::APPLIED_VAR = Exp("#applied-var");
Exp const Sig::APPLIED_CONST = Exp("#applied-const");
Exp const Sig::MISSING_TERM = Exp("#missing-term");
Exp const Sig::UNAPPLIED_FUN = Exp("#unapplied-fun");
Exp const Sig::NIL = Exp("#nil");

optional<Exp> Sig::reads_term( ExpReader& reader ) const {
	if( auto sym = reader.reads_sym() ) {
		auto info = find(*sym);
		if( info && info->arity() != 0 ) {
			throw Error(UNAPPLIED_FUN(*sym));
		}
		return *sym;
	}
	if( reader.opens() ) {
		auto const& fun = reader.reads_sym();
		if( !fun ) {
			throw Error(NIL);
		}
		auto const& info = find(*fun);
		if( !info ) {
			throw Error(APPLIED_VAR(*fun));
		}
		unsigned char arity = info->arity();
		if( arity == 0 ) {
			throw Error(APPLIED_CONST(*fun));
		}
		vector<Exp> args;
		for( unsigned char n = 0; n < arity; n++ ) {
			auto const& arg = reads_term(reader);
			if( !arg ) {
				throw Error(MISSING_TERM(Exp(*fun)(std::move(args))));
			}
			args.push_back(*arg);
		}
		if( !reader.closes() ) {
			throw Error(MISSING_RPAR(Exp(*fun)(std::move(args))));
		}
		return Exp(*fun)(std::move(args));
	}
	return nullopt;
}