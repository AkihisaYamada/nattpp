#include "trs.hpp"

using namespace std;

void Trs::insert_rule( uint32_t i, Rule const& rule ) & {
	auto const& [l,r,w] = rule;
	auto [ref,fl] = rules.emplace(i,rule);
	assert(fl);
	ASSERTED(sig.find(l.fun()))->defined_by.emplace(i);
}

void Trs::erase_rule( uint32_t i ) & {
	auto const& [l,r,w] = *ASSERTED(rules.find(i));
	ASSERTED(sig.find(l.fun()))->defined_by.erase(i);
	rules.erase(i);
}

std::ostream& Trs::Rank::print_content( std::ostream& os, bool definers ) const& {
	os << (int)arity;
	if( definers ) {
		os << " :defined_by (" << print_list(defined_by) << ')';
	}
	return os;
}

std::ostream& Trs::Rule::print_content( std::ostream& os ) const& {
	os << first << ' ' << second;
	if( weight != 1 ) {
		os << " :cost " << weight;
	}
	return os;
}
std::ostream& Trs::print_sig( std::ostream& os, Sig const& sig, std::string_view const& sep ) {
	for( auto const& [f,rank] : sig ) {
		os << sep << "(fun " << f << ' ' << rank.print_content(true) << ')' << std::flush;
	}
	return os;
}
std::ostream& Trs::print_rules( std::ostream& os, Rules const& rules, std::string_view const& sep ) {
	for( auto const& [n,rule] : rules ) {
		os << sep << "(rule " << rule.print_content() << " :number " << n << ')' << std::flush;
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