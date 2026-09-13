#include <cassert>
#include <sstream>
#include "util.hpp"
#include "exp.hpp"

using namespace std;

Answer const Answer::YES = Answer("YES"), Answer::NO = Answer("NO"), Answer::MAYBE("MAYBE");

ostream& operator<<( ostream& os, Pos const& pos ) {
	return os << '(' << print_list( pos.begin(), pos.end(), []( auto c ){ return (unsigned int)(*c)+1; } ) << ')';
}

Opt<unsigned long> nat_of( string_view const& str ) {
	unsigned long val = 0;
	unsigned int pos = 0;
	for(;;) {
		auto c = str[pos];
		if( c == '.' ) {
			for(;;) {
				pos++;
				if( str[pos] == '0' ) return {};
				if( pos == str.length() ) return {val};
			}
		}
		if( c < '0' || '9' < c ) return {};
		val = val * 10 + c - '0';
		pos++;
		if( pos == str.length() ) return {val};
	}
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
string Reader::_read_string_literal() & {
	string str = "\"";
	for(;;) {
		switch( auto c = _is.get() ) {
		case '\"': str.push_back('\"'); return str;
		case '\\':
			switch( auto e = _is.get() ) {
			case '\"': str.push_back(e); break;
			case '\\': str.push_back(e); break;
			case 'n': str.push_back('\n'); break;
			case 'r': str.push_back('\r'); break;
			case 't': str.push_back('\t'); break;
			default: throw error( "#unsupported-escape", string("\\")+(char)e );
			} break;
		default: str.push_back(c); break;
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
				_fetched = Str(_read_string_literal());
				return;
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
	if( auto key = reads_key() ) {// keys are treated as symbols
		return Exp(*key);
	}
	if( auto str = reads_str() ) {// string literals are treated as symbols
		return Exp(*str);
	}
	if( auto sym = reads_sym() ) {
		return Exp(std::move(*sym));
	}
	if( opens() ) {
		if( closes() ) {
			return Exp("()");
		}
		auto fun = read_sym();
		vector<Term<string>> args;
		while( !closes() ) {
			args.push_back(read_exp());
		}
		return {app(fun,std::move(args))};
	}
	return {};
}

void Exp::process_keys( size_t& i, KeyValProc const& f ) const& {
	size_t n = args().size();
	while( i < n ) {
		auto const& key = is_key(arg(i)).value_or_throw(Error("#unexpected",arg(i)));
		i++;
		if( i == n ) throw Error("#missing-value",key);
		auto const& val = arg(i);
		if( !f(key,val) ) throw Error("#unprocessed-key",key,val);
		i++;
	}
}

Exp Exp::of( string const& str ) {
	std::istringstream is(str);
	Reader reader(is);
	auto ret = reader.read_exp();
	if( !reader.eof() ) throw reader.error("#unexpected");
	return ret;
}

void Exp::test() {
	cout << Exp("foo") << endl;
	cout << Exp("foo","bar") << endl;
	auto t = Exp("foo",Exp("bar","buz"));
	cout << t << endl;
	assert( Exp::of("( foo; comment\n(bar  buz)") == t );
}