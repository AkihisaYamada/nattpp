#include "trs.hpp"

using namespace std;

ostream& Trs::Rule::print_contents( ostream& os ) const {
	os << first << ' ' << second;
	if( weight != 1 ) {
		os << " :cost " << weight;
	}
	return os;
}

Opt<Trs::Exp> Trs::Reader::reads() {
	if( auto sym = _reader.reads_sym() ) {
		if( auto info = _sig.find(*sym) ) {
			if( info->arity != 0 ) {
				throw Error{"#unapplied-fun",std::move(*sym)};
			}
			return Exp{*sym};
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
		Exp ret = *fun;
		for( unsigned char n = 0; n < arity; n++ ) {
			auto const& arg = reads();
			if( !arg ) {
				throw Error{"#too-few-args",ret};
			}
			ret.args().push_back(*arg);
		}
		if( !_reader.closes() ) {
			throw Error{"too-many-args",ret};
		}
		return ret;
	}
	return {};
}