#include <cassert>
#include "exp.hpp"

using namespace std;

bool ExpReader::opens() {
	_is >> ws;
	if( _is.peek() == '(' ) {
		_is.get();
		return true;
	}
	return false;
}
bool ExpReader::closes() {
	_is >> ws;
	if( _is.peek() == ')' ) {
		_is.get();
		return true;
	}
	return false;
}
void ExpReader::close() {
	if( !closes() ) {
		throw ParseError("expected close");
	}
}
optional<String> ExpReader::reads_sym() {
	_is >> ws;
	switch( _is.peek() ) {
	case '(': case ')': case ':': case '"': case '\'':
		return nullopt;
	default:
		for( string str(1,_is.get()); ; ) {
			switch( _is.peek() ) {
			case ' ': case '\t': case '\n': case '\r':
			case '(': case ')': case EOF:
				return _dict.touch(str);
			default:
				str.push_back(_is.get());
				continue;
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
				throw ParseError(std::move(what));
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

