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

class App;
class Exp {
	std::variant<String,Ptr<App const>> _un;
	/**
	 * @brief optional reference.
	 * 
	 */
	template<typename T>
	class OptRef {
		T* ptr;
	public:
		OptRef(T* ptr) : ptr(ptr) {}
		T& operator*() const { return *ptr; }
		T* operator->() const { return ptr; }
		operator bool() const { return ptr; }
	};
	Exp() = delete;
public:
	Exp( String const& str ) : _un(str) {}
	Exp( App const& app ) : _un(app) {}
	OptRef<String const> sym() const {
		return OptRef(std::get_if<String>(&_un));
	}
	OptRef<App const> app() const {
		auto const& p = std::get_if<Ptr<App const>>(&_un);
		return OptRef( p ? &**p : nullptr );
	}
};
class App {
	App() = delete;
public:
	Exp fun;
	std::vector<Exp> args;
	App( Exp const& fun, std::vector<Exp>&& args ) : fun(fun), args(std::move(args)) {}
	App( Exp const& fun, std::initializer_list<Exp> args ) : fun(fun), args(std::vector<Exp>(args)) {}
};

inline bool operator==( Exp const& l, std::string_view r ) {
	auto sym = l.sym();
	return sym && *sym == r;
}

struct ExpError : std::exception {
	Exp msg;
	ExpError(Exp const& msg) : msg(msg) {}
};

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
};

extern Exp const MISSING_SYMBOL;
extern Exp const MISSING_EXP;
extern Exp const MISSING_CLOSE;

class ExpReader {
	Dict& _dict;
	std::istream& _is;
public:
	ExpReader( std::istream& is, Dict& dict ) : _is(is), _dict(dict) {}
	bool opens();
	bool closes();
	void close();
	std::optional<String> reads_key();
	std::optional<String> reads_sym();
	String read_sym() {
		auto sym = reads_sym();
		if( !sym ) {
			throw ExpError(MISSING_SYMBOL);
		}
		return *sym;
	}
	int read_int() {
		return std::stoi(read_sym());
	}
	std::optional<Exp> reads_exp();
	Exp read_exp() {
		auto exp = reads_exp();
		if( !exp ) {
			throw ExpError(MISSING_EXP);
		}
		return *exp;
	}
};

std::ostream& operator<<( std::ostream& os, Exp const& e );

#endif
