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
	typedef std::pair<Exp,std::vector<Exp>> App;
	Sum<std::string,Mem<App const>> _un;
	static Exp _construct( std::initializer_list<Exp> list );
public:
	struct Error;
	class Reader;
	Exp( Exp const& other ) = default;
	Exp( Exp && other ) = default;
	/** @brief "nil" */
	explicit Exp() {}
	/** @brief Symbol */
	Exp( std::string && str ) : _un(std::move(str)) {}
	/** @brief Symbol */
	Exp( std::string const& str ) : _un(str) {}
	/** @brief Symbol */
	Exp( std::string_view const& str ) : _un(std::in_place_type<std::string>,str) {}
	/** @brief Symbol */
	Exp( char const* str ) : _un(str) {}
	/** @brief Number as symbol */
	Exp( int n ) : _un(std::to_string(n)) {}
	/** @brief Application */
	explicit Exp( Exp const& fun, std::vector<Exp>&& args ) :
		_un(Mem<App const>::make(fun,std::move(args))) {}
	/** @brief Application */
	explicit Exp( Exp const& fun, std::initializer_list<Exp> args ) :
		_un(Mem<App const>::make(fun,args)) {}
	/** @brief Application */
	template<typename It> requires std::input_iterator<It>
	explicit Exp( Exp const& fun, It begin, It end) :
		_un(Mem<App const>::make(std::piecewise_construct,std::tuple<Exp>(fun),std::tuple<It,It>(begin,end))) {}
	/** @brief For handy construction of applications. */
	Exp( std::initializer_list<Exp> list ) : Exp(_construct(list)) {}
	/**
	 * @brief Fast conditional reference to the string of a symbol expression.
	 * @return r such that (bool)r is true iff this is a symbol, and *r is the string.
	 */
	Opt<std::string const&> sym() const & {
		return _un.ref<std::string>();
	}
	/**
	 * @brief Conditional access to the string of a symbol expression.
	 * @return an option that contains the string iff this is a symbol.
	 */
	Opt<std::string> sym() && {
		return std::move(_un).ref<std::string>();
	}
	/**
	 * @brief Fast conditional reference to the body of an application.
	 */
	Opt<App const&> app() const & {
		if( auto p = _un.ref<Mem<App const>>() ) {
			return **p;
		} else {
			return {};
		}
	}
	/**
	 * @brief Conditional reference to the body of an application.
	 */
	Opt<App> app() const && {
		if( auto p = _un.ref<Mem<App const>>() ) {
			return std::move(**p);
		} else {
			return {};
		}
	};
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
