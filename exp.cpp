#include <cassert>
#include "exp.hpp"

using namespace std;

Exp Exp::_construct( initializer_list<Exp> list ) {
	auto it = list.begin(), end = list.end();
	if( it == end ) {
		return Exp();
	} else {
		Exp fun = *it;
		it++;
		return Exp(fun,vector<Exp>(it,end));
	}
}

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
		return Exp(*sym);
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
				return Exp(fun,std::move(args));
			} else {
				std::string what;
				_is >> what;
				throw Error(Exp(std::move(what)));
			}
		}
	}
	return {};
}

std::ostream& operator<<(std::ostream& os, Exp const& e) {
	if( auto sym = e.sym() ) {
		os << *sym;
	} else if( auto app = e.app() ) {
		os << '(' << app->first;
		for( auto arg : app->second ) {
			os << ' ' << arg;
		}
		os << ')';
	} else {
		assert(false);
	}
	return os;
}

