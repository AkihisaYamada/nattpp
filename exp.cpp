#include <cassert>
#include "exp.hpp"

using namespace std;

std::optional<Exp> Dict::reads_exp(std::istream& is) {
	is >> std::ws;
	switch( is.peek() ) {
	case '(':
		is.get();
		for(std::vector<Exp> list;;) {
			auto e = reads_exp(is);
			if( e.has_value() ) {
				list.push_back(*e);
			} else if( is.peek() == ')' ) {
				is.get();
				return Exp(std::move(list));
			} else {
				std::string what;
				is >> what;
				throw Exp::ParseError(std::move(what));
			}
		}
	case ')': case EOF:
		return std::optional<Exp>();
	default:
		for( std::string str(1,is.get()); ; ) {
			switch( is.peek() ) {
			case ' ': case '\t': case '\n': case '\r': case '(': case ')': case EOF:
				return Exp(touch(str));
			default:
				str.push_back(is.get());
				continue;
			}
		}
	}
	return std::optional<Exp>();
}

std::ostream& operator<<(std::ostream& os, Exp const& e) {
	e.cases<void>([&](String const& sym){
		os << sym;
	}, [&](auto const& app) {
		os << '(';
		if( auto it = app.begin(); it != app.end() ) {
			os << *it;
			it++;
			for( ; it != app.end(); it++ ) {
				os << ' ' << *it;
			}
		}
		os << ')';
	});
	return os;
}

