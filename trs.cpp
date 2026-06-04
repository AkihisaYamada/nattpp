#include "trs.hpp"

using namespace std;

static ostream& _print_rule_inner( ostream& os, Trs::Rule const& rule ) {
	os << rule.first << ' ' << rule.second;
	if( rule.weight != 1 ) {
		os << " :cost " << rule.weight;
	}
	return os << ')';
}
ostream& Trs::Rule::print( ostream& os ) const {
	return _print_rule_inner( os << "(rule ", *this );
}
ostream& Trs::Rule::print( ostream& os, int index ) const {
	return _print_rule_inner( os << "(rule-no " << index << ' ', *this );
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