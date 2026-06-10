#include "trs.hpp"

using namespace std;

ostream& Trs::Rule::print_contents( ostream& os ) const {
	os << first << ' ' << second;
	if( weight != 1 ) {
		os << " :cost " << weight;
	}
	return os;
}

Opt<Trs::Term> Trs::Reader::reads() {
	if( auto sym = _reader.reads_sym() ) {
		if( auto info = _sig.find(*sym) ) {
			if( info->arity != 0 ) {
				throw Error{"#unapplied-fun",std::move(*sym)};
			}
			return Term{*sym};
		}
		return *sym;
	}
	if( _reader.opens() ) {
		auto const& fun = _reader.reads_sym();
		if( !fun ) {
			throw Error("#nil");
		}
		auto const& info = _sig.find(*fun);
		if( !info ) {
			throw Error{"#applied-var",*fun};
		}
		unsigned char arity = info->arity;
		if( arity == 0 ) {
			throw Error{"#applied-const",*fun};
		}
		vector<::Term<string>> args;
		for( unsigned char n = 0; n < arity; n++ ) {
			args.push_back( reads().value_or_throw(Error{"#too-few-args",app(*fun,args)}) );
		}
		auto ret = app(*fun,args);
		if( !_reader.closes() ) throw Error{"too-many-args",ret};
		return ret;
	}
	return {};
}