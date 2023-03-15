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

#include"ref.hpp"

class Exp {
	class _App;
	std::variant<std::string,Ptr<_App const>> _un;
	static Exp _construct( std::initializer_list<Exp> const& list );
public:
	/**
	 * @brief Application. The pair of the function and the vector of arguments.
	 */
	typedef std::pair<Exp,std::vector<Exp>> App;
	/**
	 * @brief "nil"
	 */
	Exp() {}
	/**
	 * @brief Symbol
	 */
	Exp( std::string const& str ) : _un(str) {}
	Exp( char const* str ) : _un(str) {}
	/**
	 * @brief Application.
	 * 
	 * @param fun 
	 * @param args 
	 */
	Exp( Exp const& fun, std::vector<Exp> && args );
	/**
	 * @brief For handy construction of applications.
	 * 
	 * @param list 
	 */
	Exp( std::initializer_list<Exp> const& list ) : Exp(_construct(list)) {}
	/**
	 * @brief Fast conditional reference to the string of a symbol expression.
	 * @return r such that (bool)r is true iff this is a symbol, and *r is the string.
	 */
	TempOpt<std::string const> sym() const & {
		if( auto p = std::get_if<std::string>(&_un) ) {
			return *p;
		} else {
			return std::nullopt;
		}
	}
	/**
	 * @brief Conditional access to the string of a symbol expression.
	 * @return an option that contains the string iff this is a symbol.
	 */
	std::optional<std::string> sym() const && {
		if( auto p = std::get_if<std::string>(&_un) ) {
			return *p;
		} else {
			return std::nullopt;
		}
	}
	/**
	 * @brief Fast conditional reference to the body of an application.
	 */
	TempOpt<App const> app() const &;
	/**
	 * @brief Conditional reference to the body of an application.
	 */
	std::optional<App> app() const &&;
	class Reader;
};

struct Exp::_App : App {};

inline Exp::Exp( Exp const& fun, std::vector<Exp>&& args ) : _un(_App({fun,std::move(args)})) {}

inline TempOpt<Exp::App const> Exp::app() const & {
	if( auto p = std::get_if<Ptr<_App const>>(&_un) ) {
		return **p;
	} else {
		return std::nullopt;
	}
}

inline std::optional<Exp::App> Exp::app() const && {
	if( auto const& p = std::get_if<Ptr<_App const>>(&_un) ) {
		return **p;
	} else {
		return std::nullopt;
	}
}

inline bool operator==( Exp const& l, std::string_view r ) {
	auto sym = l.sym();
	return sym && *sym == r;
}

class Exp::Reader {
	std::istream& _is;
	class LPar {};
	class RPar {};
	class None {};
	struct Key { std::string str; };
	struct Sym { std::string str; };
	std::variant<None,LPar,RPar,Key,Sym> _fetched;
	void _fetch();
public:
	struct Error : std::exception {
		Exp msg;
		Error(Exp const& msg) : msg(msg) {}
	};
	Reader( std::istream& is ) : _is(is), _fetched(None()) {}
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
			throw Error("#missing-left-paren");
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
			throw Error("#missing-right-paren");
		}
	}
	std::optional<std::string> reads_key() {
		_fetch();
		if( auto key = std::get_if<Key>(&_fetched) ) {
			std::string str = key->str;
			_fetched.emplace<None>();
			return str;
		}
		return std::nullopt;
	}
	std::optional<std::string> reads_sym() {
		_fetch();
		if( auto sym = std::get_if<Sym>(&_fetched) ) {
			std::string str = sym->str;
			_fetched.emplace<None>();
			return str;
		}
		return std::nullopt;
	}
	std::string read_sym() {
		auto sym = reads_sym();
		if( !sym ) {
			throw Error("#missing-symbol");
		}
		return *sym;
	}
	bool reads_sym( char const* str ) {
		_fetch();
		if( auto sym = std::get_if<Sym>(&_fetched) ) {
			if( str == sym->str ) {
				_fetched.emplace<None>();
				return true;
			}
		}
		return false;
	}
	void read_sym( char const* str ) {
		if( !reads_sym(str) ) {
			throw Error(Exp("#missing-symbol",{str}));
		}
	}
	int read_int() {
		return std::stoi(read_sym());
	}
	std::optional<Exp> reads_exp();
	Exp read_exp() {
		auto exp = reads_exp();
		if( !exp ) {
			throw Error("#missing-expression");
		}
		return *exp;
	}
};

std::ostream& operator<<( std::ostream& os, Exp const& e );

#endif
