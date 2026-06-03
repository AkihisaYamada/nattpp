#include <cassert>
#include "exp.hpp"

using namespace std;

static void skip_line( istream& is ) {
	for(;;) {
		switch( is.get() ) {
		case '\n': case '\r': case EOF:
			break;
		default:
			continue;
		}
	}
}
static string read_sym_rest( istream& is, int c ) {
	string str = string(1,c);
	for(;;) {
		switch( c = is.peek() ) {
		case ' ': case '\t': case '\n': case '\r':
			is.ignore();
			return str;
		case ';': case '(': case ')': case EOF:
			return str;
		default:
			str.push_back(c);
			is.ignore();
			continue;
		}
	}
}

Opt<unsigned int> Reader::reads_nat() {
	_fetch();
	if( auto sym = _fetched.ref<Sym>() ) {
		unsigned int val = 0;
		for( auto c : sym->str ) {
			if( c < '0' || '9' < c ) return {};
			val = 10 * val + c - '0';
		}
		_fetched = None();
		return {val};
	}
	return {};
}


void Reader::_fetch() {
	if( _fetched.ref<None>() ) {
		for(;;) {
			switch( int c = _is.get() ) {
			case ' ': case '\t': case '\n': case '\r':// skip white spaces
				continue;
			case ';':// skip comment line
				skip_line(_is);
				continue;
			case '(':
				_fetched = LPar();
				return;
			case ')':
				_fetched = RPar();
				return;
			case '"':
				throw Error("unsupported symbol (\")");
			case '\'':
				throw Error("unsupported symbol (')");
			case ':':
				_fetched = Key(read_sym_rest(_is,c));
				return;
			case EOF:
				_fetched = None();
				return;
			default:
				_fetched = Sym(read_sym_rest(_is,c));
				return;
			}
		}
	}
}

Opt<Exp> Reader::reads_exp() {
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

int Exp::test() {
	cout << Exp("foo") << endl;
	cout << Exp("foo","bar") << endl;
	cout << Exp("foo",Exp("bar","buz")) << endl;
	return 0;
}