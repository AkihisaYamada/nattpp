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

template<typename F>
class Term {
	typedef F Fun;
	/**
	 * @brief Application. The pair of the function and the vector of arguments.
	 */
	typedef std::pair<Fun,std::vector<Term>> App;
	Mem<App> _mem;
public:
	/** @brief copy constructor */
	Term( Term const& other ) = default;
	/** @brief move constructor */
	Term( Term && other ) : _mem(std::move(other._mem)) {}
	/** @brief Application */
	template<typename S, typename... Args> requires (
		std::is_constructible_v<F,S> &&
		(std::is_constructible_v<Term,Args> && ...)
	)
	Term( S const& fun, Args const&... args ) :
		_mem(App(fun,{args...})) {}
	/**
	 * @brief accesses the function
	 */
	F& fun() & {
		return _mem->first;
	}
	/**
	 * @brief accesses the function
	 */
	F const& fun() const & {
		return _mem->first;
	}
	/**
	 * @brief accesses the arguments
	 */
	std::vector<Term>& args() & {
		return _mem->second;
	};
	/**
	 * @brief accesses the arguments
	 */
	std::vector<Term> const& args() const & {
		return _mem->second;
	};
	Term& operator=( Term && other ) & {
		_mem = std::move(other._mem);
		return *this;
	}
	Term& operator=( Term const& other ) & {
		_mem = other._mem;
		return *this;
	}
	bool operator==( Term const& other ) const {
		return _mem == other._mem;
	}
};

using Exp = Term<std::string>;

struct Error : std::exception, Exp {
	using Exp::Exp;
};

class Reader {
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

template<typename F>
std::ostream& operator<<( std::ostream& os, Term<F> const& e ) {
	auto const& fun = e.fun();
	auto const& args = e.args();
	if( args.empty() ) {
		return os << fun;
	}
	os << '(' << fun;
	for( auto arg : args ) {
		os << ' ' << arg;
	}
	return os << ')';
}


#endif
