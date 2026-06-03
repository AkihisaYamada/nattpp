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

Opt<unsigned int> nat_of( std::string_view const& str );

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
	template<typename S> requires std::is_constructible_v<F,S>
	Term( std::in_place_t const&, S const& fun, std::vector<Term>&& args ) :
		_mem(App(fun,std::move(args))) {}
	template<typename S> requires std::is_constructible_v<F,S>
	static Term app( S const& fun, std::vector<Term>&& args ) {
		return Term(std::in_place,fun,std::move(args));
	}
	/** @brief Application */
	template<typename S, typename... Args> requires (
		std::is_constructible_v<F,S> &&
		(std::is_constructible_v<Term,Args> && ...)
	)
	Term( S const& fun, Args const&... args ) :
		_mem(App(fun,{Term(args)...})) {}
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
	template<typename G>
	Term<G> map( std::function<G(F const&)> f ) const {
		Term<G> ret = f(fun());
		for( auto& arg : args() ) {
			ret.args().push_back(arg.map(f));
		}
		return ret;
	}
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

struct Exp : Term<std::string> {
	using Term<std::string>::Term;
	Exp( Term && other ) : Term<std::string>(std::move(other)) {}
	Exp( Term const& other ) : Term<std::string>(other) {}
	static int test();
};

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
	unsigned int _line = 1;
	unsigned int _column = 1;
	unsigned int _fetched_column = 1;
	std::string _string_fetched() const& {
		if( _fetched.ref<LPar>() ) return "#lparen";
		if( _fetched.ref<RPar>() ) return "#rparen";
		if( auto key = _fetched.ref<Key>() ) return key->str;
		if( auto sym = _fetched.ref<Sym>() ) return sym->str;
		return "#none";
	}
	Error _err( std::string const& msg ) const& {
		return Error(msg,":line",std::to_string(_line),":column",std::to_string(_column),":encount",_string_fetched());
	}
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
		if( !opens() ) throw _err("#missing-left-paren");
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
		if( !closes() ) throw _err("#missing-right-paren");
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
		if( !sym ) throw _err("#missing-symbol");
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
	Opt<unsigned int> reads_nat() {
		_fetch();
		if( auto sym = _fetched.ref<Sym>() )
		if( auto val = nat_of(sym->str) ) {
			_fetched = None();
			return val;
		}
		return {};
	}
	void read_sym( char const* str ) {
		if( !reads_sym(str) ) {
			throw Error{"#missing-symbol",str};
		}
	}
	unsigned int read_nat() {
		auto opt = reads_nat();
		if( !opt ) throw _err("#missing-number");
		return *opt;
	}
	int read_int() {
		return std::stoi(read_sym());
	}
	Opt<Exp> reads_exp();
	Exp read_exp() {
		auto exp = reads_exp();
		if( !exp ) throw _err("#missing-expression");
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
