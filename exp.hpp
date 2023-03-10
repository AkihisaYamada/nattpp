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
	App( Exp const& fun, std::vector<Exp>&& args ) : fun(fun), args(args) {}
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

extern Exp const MISSING_SYM;
extern Exp const MISSING_EXP;
extern Exp const MISSING_LPAR;
extern Exp const MISSING_RPAR;

class ExpReader {
	Dict& _dict;
	std::istream& _is;
	class LPar {};
	class RPar {};
	class None {};
	struct Key { String const& str; };
	struct Sym { String const& str; };
	std::variant<None,LPar,RPar,Key,Sym> _fetched;
	void _fetch();
public:
	ExpReader( std::istream& is, Dict& dict ) : _is(is), _dict(dict), _fetched(None()) {}
	bool opens() {
		_fetch();
		if( std::get_if<LPar>(&_fetched) ) {
			_fetched.emplace<None>();
			return true;
		}
		return false;
	}
	void open() {
		if( !opens() ) {
			throw ExpError(MISSING_LPAR);
		}
	}
	bool closes() {
		_fetch();
		if( std::get_if<RPar>(&_fetched) ) {
			_fetched.emplace<None>();
			return true;
		}
		return false;
	}
	void close() {
		if( !closes() ) {
			throw ExpError(MISSING_RPAR);
		}
	}
	std::optional<String> reads_key() {
		_fetch();
		if( auto key = std::get_if<Key>(&_fetched) ) {
			String str = key->str;
			_fetched.emplace<None>();
			return str;
		}
		return std::nullopt;
	}
	std::optional<String> reads_sym() {
		_fetch();
		if( auto sym = std::get_if<Sym>(&_fetched) ) {
			String str = sym->str;
			_fetched.emplace<None>();
			return str;
		}
		return std::nullopt;
	}
	String read_sym() {
		auto sym = reads_sym();
		if( !sym ) {
			throw ExpError(MISSING_SYM);
		}
		return *sym;
	}
	bool reads_sym( String const& str ) {
		_fetch();
		if( auto sym = std::get_if<Sym>(&_fetched) ) {
			if( str == sym->str ) {
				_fetched.emplace<None>();
				return true;
			}
		}
		return false;
	}
	void read_sym( String const& str ) {
		if( !reads_sym(str) ) {
			throw ExpError(App{MISSING_SYM,{str}});
		}
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
