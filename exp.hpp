#ifndef _EXP_HPP
#define _EXP_HPP

#include<cassert>
#include<string>
#include<vector>
#include<set>
#include<optional>
#include<functional>
#include<iostream>
#include<exception>
#include<variant>

#include"string.hpp"

class Exp {
	std::variant<String,Ref<std::vector<Exp>const>> _un;
public:
	struct ParseError : std::exception {
		std::string const& str;
		ParseError(std::string const&& str) : str(str) {}
	};
	struct ExpectsSym : std::exception {
	};
	struct ExpectsApp : std::exception {
	};
	Exp( String const& str ) : _un(str) {}
	Exp( std::vector<Exp> const&& vec ) : _un(vec) {}
	Exp( std::initializer_list<Exp> list ) : _un(std::vector<Exp>(list)) {}
	String const& sym() const {
		if( auto const& p = std::get_if<String>(&_un) ) {
			return *p;
		}
		throw ExpectsSym();
	}
	std::vector<Exp> const& app() const {
		if( auto const& p = std::get_if<Ref<std::vector<Exp> const>>(&_un) ) {
			return **p;
		}
		throw ExpectsApp();
	}
	template<class T>
	T cases(
		std::function<T(String const&)> sym,
		std::function<T(std::vector<Exp> const&)> app
	) const {
		if( auto const& p = std::get_if<String>(&_un) ) {
			return sym(*p);
		}
		if( auto const& p = std::get_if<Ref<std::vector<Exp> const>>(&_un) ) {
			return app(**p);
		}
		assert(false);
	}
};
inline bool operator==( Exp const& l, std::string_view r ) {
	return l.cases<bool>(
		[&](String const& l2){ return l2 == r; },
		[&](std::vector<Exp> const& l2){ return false; }
	);
}
class Dict {
	std::set<String,std::less<>> _set;
public:
	String const& touch( String const& str ) {
		return *_set.insert(str).first;
	}
	String const& touch( std::string const& str ) {
		return *_set.insert(str).first;
	}
	String const& touch( char const* str ) {
		return *_set.insert(str).first;
	}

	std::optional<Exp> reads_exp(std::istream& is);
};

std::ostream& operator<<(std::ostream& os, Exp const& e);

#endif
