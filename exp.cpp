#include <cassert>
#include "exp.hpp"

using namespace std;

static void skip_comment_line( istream& is ) {
	is.get();
	for(;;) {
		switch( is.get() ) {
		case '\n': case '\r': case EOF:
			break;
		default:
			continue;
		}
	}
}
static string read_sym_rest( istream& is ) {
	string str = string(1,is.get());
	for(;;) {
		switch( is.peek() ) {
		case ' ': case '\t': case '\n': case '\r': case ';':
		case '(': case ')': case EOF:
			return str;
		default:
			str.push_back(is.get());
			continue;
		}
	}
}
void Exp::Reader::_fetch() {
	if( _fetched.ref<None>() ) {
		for(;;) {
			switch( _is.peek() ) {
			case ' ': case '\t': case '\n': case '\r':// skip white spaces
				_is.get();
				continue;
			case ';':// skip comment line
				skip_comment_line(_is);
				continue;
			case '(':
				_is.get();
				_fetched = LPar();
				return;
			case ')':
				_is.get();
				_fetched = RPar();
				return;
			case '"': case '\'':
				return;
			case ':':
				_fetched = Key(read_sym_rest(_is));
				return;
			case EOF:
				_fetched = None();
				return;
			default:
				_fetched = Sym(read_sym_rest(_is));
				return;
			}
		}
	}
}

Opt<Exp> Exp::Reader::reads_exp() {
	if( auto sym = reads_sym() ) {
		return Exp(std::move(*sym));
	}
	if( auto key = reads_key() ) {// keys are treated as symbols
		return Exp(*key);
	}
	if( opens() ) {
		if( closes() ) {
			return Exp("()");
		}
		Exp ret = read_sym();
		while( !closes() ) {
			ret.args().push_back(read_exp());
		}
		return ret;
	}
	return {};
}

std::ostream& operator<<(std::ostream& os, Exp const& e) {
	auto const& fun = e.fun();
	auto const& args = e.args();
	if( args.empty() ) {
		return os << fun;
	}
	os << '(' << fun;
	for( auto arg : e.args() ) {
		os << ' ' << arg;
	}
	return os << ')';
}

