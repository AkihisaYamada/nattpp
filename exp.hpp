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
	/**
	 * @brief Application. first is the function, and second is the list of arguments.
	 */
	class App;
	std::variant<std::string,Ptr<App const>> _un;
	Exp() = delete;
public:
	Exp( std::string const& str ) : _un(str) {}
	Exp( char const* str ) : _un(str) {}
	Exp( Exp const& fun, std::vector<Exp>&& args );
	Exp operator()( Exp const& arg ) const;
	Exp operator()( std::vector<Exp>&& args ) const;
	Exp operator()( std::initializer_list<Exp> args ) const;
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
	TempOpt<App const> app() const & {
		if( auto p = std::get_if<Ptr<App const>>(&_un) ) {
			return **p;
		} else {
			return std::nullopt;
		}
	}
	/**
	 * @brief Conditional reference to the body of an application.
	 */
	std::optional<App> app() const &&;
};

struct Exp::App : std::pair<Exp,std::vector<Exp>> {};

inline Exp::Exp( Exp const& fun, std::vector<Exp>&& args ) : _un(App({fun,args})) {}

inline std::optional<Exp::App> Exp::app() const && {
	if( auto const& p = std::get_if<Ptr<App const>>(&_un) ) {
		return **p;
	} else {
		return std::nullopt;
	}
}

inline Exp Exp::operator()( std::vector<Exp>&& args ) const {
	return Exp(*this,std::move(args));
}
inline Exp Exp::operator()( std::initializer_list<Exp> args ) const {
	return Exp(*this,args);
}
inline Exp Exp::operator()( Exp const& arg ) const {
	return Exp(*this,{arg});
}
inline bool operator==( Exp const& l, std::string_view r ) {
	auto sym = l.sym();
	return sym && *sym == r;
}

struct ExpError : std::exception {
	Exp msg;
	ExpError(Exp const& msg) : msg(msg) {}
};

extern Exp const MISSING_SYM;
extern Exp const MISSING_EXP;
extern Exp const MISSING_LPAR;
extern Exp const MISSING_RPAR;

class ExpReader {
	std::istream& _is;
	class LPar {};
	class RPar {};
	class None {};
	struct Key { std::string str; };
	struct Sym { std::string str; };
	std::variant<None,LPar,RPar,Key,Sym> _fetched;
	void _fetch();
public:
	ExpReader( std::istream& is ) : _is(is), _fetched(None()) {}
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
			throw ExpError(MISSING_SYM);
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
			throw ExpError(MISSING_SYM({str}));
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
