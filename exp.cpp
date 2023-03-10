#include <cassert>
#include "exp.hpp"

using namespace std;

Exp const MISSING_SYM = Exp("#missing_symbol");
Exp const MISSING_EXP = Exp("#missing_expression");
Exp const MISSING_LPAR = Exp("#missing_left_paren");
Exp const MISSING_RPAR = Exp("#missing_right_paren");

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
static String const& read_sym_rest( istream& is, Dict& dict ) {
	string str = string(1,is.get());
	for(;;) {
		switch( is.peek() ) {
		case ' ': case '\t': case '\n': case '\r': case ';':
		case '(': case ')': case EOF:
			return dict.touch(str);
		default:
			str.push_back(is.get());
			continue;
		}
	}
}
void ExpReader::_fetch() {
	if( get_if<None>(&_fetched) ) {
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
				_fetched.emplace<LPar>();
				return;
			case ')':
				_is.get();
				_fetched.emplace<RPar>();
				return;
			case '"': case '\'':
				return;
			case ':':
				_fetched.emplace<Key>(read_sym_rest(_is,_dict));
				return;
			default:
				_fetched.emplace<Sym>(read_sym_rest(_is,_dict));
				return;
			}
		}
	}
}

optional<Exp> ExpReader::reads_exp() {
	if( auto sym = reads_sym() ) {
		return *sym;
	}
	if( _is.peek() == '(' ) {
		_is.get();
		Exp fun = read_exp();
		vector<Exp> args;
		for(;;) {
			if( auto e = reads_exp() ) {
				args.push_back(*e);
			} else if( _is.peek() == ')' ) {
				_is.get();
				return App(fun,std::move(args));
			} else {
				std::string what;
				_is >> what;
				throw ExpError(Exp(std::move(what)));
			}
		}
	}
	return nullopt;
}

std::ostream& operator<<(std::ostream& os, Exp const& e) {
	if( auto sym = e.sym() ) {
		os << *sym;
	} else if( auto app = e.app() ) {
		os << '(' << app->fun;
		for( auto arg : app->args ) {
			os << ' ' << arg;
		}
		os << ')';
	} else {
		assert(false);
	}
	return os;
}

