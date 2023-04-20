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
#include"sum.hpp"

class Exp {
	/**
	 * @brief Application. The pair of the function and the vector of arguments.
	 */
	typedef std::pair<std::string,std::vector<Exp>> App;
	Mem<App> _mem;
public:
	struct Error;
	class Reader;
	/** @brief copy constructor */
	Exp( Exp const& other ) = default;
	/** @brief move constructor */
	Exp( Exp && other ) : _mem(std::move(other._mem)) {}
	/** @brief Number as symbol */
	Exp( int n ) : _mem(App(std::to_string(n),{})) {}
	/** @brief Symbol */
	template<typename S>
		requires std::is_constructible_v<std::string,S>
	Exp( S const& fun ) : _mem(App(fun,{})) {}
	/** @brief Application */
	template<typename S, typename... Args> requires (
		std::is_constructible_v<std::string,S> &&
		(std::is_constructible_v<Exp,Args> && ...)
	)
	explicit Exp( S const& fun, Exp const& a1, Args const&... args ) :
		_mem(App(fun,{a1,args...})) {}
	/**
	 * @brief accesses the function
	 */
	std::string& fun() & {
		return _mem->first;
	}
	/**
	 * @brief accesses the function
	 */
	std::string const& fun() const & {
		return _mem->first;
	}
	/**
	 * @brief accesses the arguments
	 */
	std::vector<Exp>& args() & {
		return _mem->second;
	};
	/**
	 * @brief accesses the arguments
	 */
	std::vector<Exp> const& args() const & {
		return _mem->second;
	};
	Exp& operator=( Exp && other ) & = default;
	Exp& operator=( Exp const& other ) & = default;
	bool operator==( Exp const& other ) const = default;
};

struct Exp::Error : std::exception, Exp {
	using Exp::Exp;
};

class Exp::Reader {
	std::istream& _is;
	class LPar {};
	class RPar {};
	class None {};
	struct Key { std::string str; };
	struct Sym { std::string str; };
	Sum<None,LPar,RPar,Key,Sym> _fetched;
	void _fetch();
public:
	Reader( std::istream& is ) : _is(is), _fetched(None()) {}
	bool opens() {
		_fetch();
		if( _fetched.ref<LPar>() ) {
			_fetched = None();
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
		if( _fetched.ref<RPar>() ) {
			_fetched = None();
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
		if( auto key = _fetched.ref<Key>() ) {
			std::string str = std::move(key->str);
			_fetched = None();
			return str;
		}
		return {};
	}
	Opt<std::string> reads_sym() {
		_fetch();
		if( auto sym = _fetched.ref<Sym>() ) {
			std::string str = std::move(sym->str);
			_fetched = None();
			return str;
		}
		return {};
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
		if( auto sym = _fetched.ref<Sym>() ) {
			if( str == sym->str ) {
				_fetched = None();
				return true;
			}
		}
		return false;
	}
	void read_sym( char const* str ) {
		if( !reads_sym(str) ) {
			throw Error{"#missing-symbol",str};
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
