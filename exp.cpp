#include <cassert>
#include "exp.hpp"

using namespace std;

Opt<unsigned int> nat_of( string_view const& str ) {
	unsigned int val = 0;
	for( auto c : str ) {
		if( c < '0' || '9' < c ) return {};
		val = val * 10 + c - '0';
	}
	return {val};
}

static void skip_line( istream& is ) {
	for(;;) {
		switch( auto c = is.get() ) {
		case '\n': case '\r': case EOF:
			return;
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

void Reader::_fetch() {
	if( _fetched.ref<None>() ) {
		for(;;) {
			int c = _is.get();
			_fetched_column++;
			switch( c ) {
			case '\n': case '\r':// skip white spaces
				_line++;
				_fetched_column = 0;
				continue;
			case ' ': case '\t':
				continue;
			case ';':// skip comment line
				skip_line(_is);
				_line++;
				_fetched_column = 0;
				continue;
			case '(':
				_fetched = LPar();
				return;
			case ')':
				_fetched = RPar();
				return;
			case '"':
				throw error("unsupported symbol (\")");
			case '\'':
				throw error("unsupported symbol (')");
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