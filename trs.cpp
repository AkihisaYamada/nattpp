#include "trs.hpp"

using namespace std;

ostream& operator<<( ostream& os, Trs::Rule const& rule ) {
	os << "(rule " << rule.first << ' ' << rule.second;
	if( rule.weight != 1 ) {
		os << " :weight " << (int)rule.weight;
	}
	return os << ')';
}

ostream& operator<<( ostream& os, Trs::Rules const& system ) {
	for( auto const& rule : system ) {
		os << '\t' << rule << endl;
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