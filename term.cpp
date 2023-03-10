#include "term.hpp"

using namespace std;

Exp const Sig::APPLIED_VAR = Exp("#applied_var");
Exp const Sig::APPLIED_CONST = Exp("#applied_const");
Exp const Sig::UNEXPECTED_CLOSE = Exp("#unexpected_close");
Exp const Sig::MISSING_TERM = Exp("#missing_term");
Exp const Sig::UNAPPLIED_FUN = Exp("#unapplied_fun");
Exp const Sig::UNIT = Exp("#unit");

optional<Exp> Sig::reads_term( ExpReader& reader ) const {
	if( auto sym = reader.reads_sym() ) {
		auto info = find(*sym);
		if( info && info->arity() != 0 ) {
			throw Error(App(UNAPPLIED_FUN,{*sym}));
		}
		return *sym;
	}
	if( reader.opens() ) {
		auto fun = reader.read_sym();
		auto info = find(fun);
		if( !info ) {
			throw Error(App(APPLIED_VAR,{fun}));
		}
		unsigned char arity = info->arity();
		if( arity == 0 ) {
			throw Error(App(APPLIED_CONST,{fun}));
		}
		vector<Exp> args;
		for( unsigned char n = 0; n < arity; n++ ) {
			args.push_back(read_term(reader));
		}
		if( !reader.closes() ) {
			throw Error(App(MISSING_CLOSE,{fun}));
		}
		return App(fun,std::move(args));
	}
	return nullopt;
}