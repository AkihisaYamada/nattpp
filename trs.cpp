#include "trs.hpp"

using namespace std;

std::ostream& Trs::Rule::print_content( std::ostream& os ) const& {
	os << first << ' ' << second;
	if( weight != 1 ) {
		os << " :cost " << weight;
	}
	return os;
}

Opt<Trs::Term> Trs::Reader::reads( function<void(string const&)> const& var ) {
	if( auto sym = _reader.reads_sym() ) {
		if( auto info = _sig(*sym) ) {
			if( info->arity != 0 ) {
				throw _reader.error("#unapplied-fun",std::move(*sym));
			}
			return Term{*sym};
		}
		var(*sym);
		return {*sym};
	}
	if( _reader.opens() ) {
		auto const& fun = _reader.reads_sym();
		if( !fun ) {
			throw _reader.error("#nil");
		}
		auto const& info = _sig(*fun);
		if( !info ) {
			throw _reader.error("#applied-var",*fun);
		}
		unsigned char arity = info->arity;
		if( arity == 0 ) {
			throw _reader.error("#applied-const",*fun);
		}
		vector<::Term<string>> args;
		for( unsigned char n = 0; n < arity; n++ ) {
			args.push_back( reads(var).value_or_throw(_reader.error("#too-few-args",app(*fun,args))) );
		}
		auto ret = app(*fun,args);
		if( !_reader.closes() ) throw _reader.error("too-many-args",ret);
		return {ret};
	}
	return {};
}