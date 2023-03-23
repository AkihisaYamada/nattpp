#ifndef _EXP_HPP
#define _EXP_HPP

#include<cassert>
#include<string>
#include<vector>
#include<set>
#include<functional>
#include<iostream>
#include<exception>

#include"ref.hpp"
#include"opt.hpp"

class Exp {
	/**
	 * @brief Application. The pair of the function and the vector of arguments.
	 */
	typedef std::pair<Exp,std::vector<Exp>> App;
	std::variant<std::string,Ptr<App const>> _un;
	static Exp _construct( std::initializer_list<Exp> list );
public:
	class Reader;
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
	Exp( Exp const& fun, std::vector<Exp> && args ) :
		_un(Ptr<App const>::make(fun,std::move(args))) {}
	/**
	 * @brief For handy construction of applications.
	 * 
	 * @param list 
	 */
	Exp( std::initializer_list<Exp> list ) : Exp(_construct(list)) {}
	/**
	 * @brief Fast conditional reference to the string of a symbol expression.
	 * @return r such that (bool)r is true iff this is a symbol, and *r is the string.
	 */
	Opt<std::string const&> sym() const & {
		return ref_if<std::string>(_un);
	}
	/**
	 * @brief Conditional access to the string of a symbol expression.
	 * @return an option that contains the string iff this is a symbol.
	 */
	Opt<std::string> sym() && {
		return ref_if<std::string>(std::move(_un));
	}
	/**
	 * @brief Fast conditional reference to the body of an application.
	 */
	Opt<App const&> app() const & {
		if( auto p = ref_if<Ptr<App const>>(_un) ) {
			return **p;
		} else {
			return nullptr;
		}
	}
	/**
	 * @brief Conditional reference to the body of an application.
	 */
	Opt<App> app() const && {
		if( auto p = ref_if<Ptr<App const>>(_un) ) {
			return std::move(**p);
		} else {
			return nullptr;
		}
	};
};

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
		if( std::holds_alternative<LPar>(_fetched) ) {
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
		if( std::holds_alternative<RPar>(_fetched) ) {
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
	Opt<std::string> reads_key() {
		_fetch();
		if( auto key = ref_if<Key>(_fetched) ) {
			std::string str = key->str;
			_fetched.emplace<None>();
			return str;
		}
		return nullptr;
	}
	Opt<std::string> reads_sym() {
		_fetch();
		if( auto sym = ref_if<Sym>(_fetched) ) {
			std::string str = sym->str;
			_fetched.emplace<None>();
			return str;
		}
		return nullptr;
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
	Opt<Exp> reads_exp();
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
