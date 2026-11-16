#ifndef _EXP_HPP
#define _EXP_HPP

#include<cassert>
#include <concepts>
#include<string>
#include <type_traits>
#include<vector>
#include<functional>
#include<iostream>
#include<sstream>
#include<exception>

#include"ref.hpp"
#include"sum.hpp"
#include"util.hpp"

using Pos = std::vector<unsigned char>;

std::ostream& operator<<( std::ostream& os, Pos const& pos );

template<typename F>
class Term {
	template<typename G>
	friend class Term;
	using Fun = F;
	/**
	 * @brief Application. The pair of the function and the vector of arguments.
	 */
	using App = std::pair<Fun,std::vector<Term>>;
	Ref<App> _ref;
public:
	/** @brief copy constructor */
	Term( Term const& other ) = default;
	/** @brief move constructor */
	Term( Term && other ) : _ref(std::move(other._ref)) {}
	template<typename S> requires std::is_constructible_v<F,S>
	Term( std::in_place_t const&, S&& fun, std::vector<Term>&& args ) :
		_ref(Ref<App>::make(std::forward<S>(fun),std::move(args))) {}
	template<typename S> requires std::is_constructible_v<F,S>
	Term( std::in_place_t const&, S&& fun, std::vector<Term>const& args ) :
		_ref(Ref<App>::make(std::forward<S>(fun),args)) {}
	/** @brief Application */
	template<typename S, typename... Args> requires (
		std::is_constructible_v<F,S> &&
		(std::is_constructible_v<Term,Args> && ...)
	)
	Term( S const& fun, Args&&... args ) :
		_ref(Ref<App>::make(fun,std::vector<Term>{Term(std::forward<Args>(args))...})) {}
	/**
	 * @brief accesses the function
	 */
	F fun() && {
		return _ref->first;
	}
	/**
	 * @brief accesses the function
	 */
	F const& fun() const & {
		return _ref->first;
	}
	/**
	 * @brief accesses the arguments
	 */
	std::vector<Term> args() && {
		return _ref->second;
	}
	/**
	 * @brief accesses the arguments
	 */
	std::vector<Term> const& args() const & {
		return _ref->second;
	}
	/** @brief as the pair of function and arguments */
	std::pair<Fun,std::vector<Term>> const& operator*() const& {
		return *_ref;
	}
	Term arg( uint i ) && {
		assert( i < args().size() );
		return args()[i];
	}
	Term const& arg( uint i ) const& {
		assert( i < args().size() );
		return args()[i];
	}
	/** Returns i-th argument and increment i. */
	Opt<Term const&> gets_arg( uint& i ) const& {
		if( i < args().size() ) {
			auto const& a = arg(i);
			if( !is_key(a) ) {
				i++;
				return {a};
			}
		}
		return {};
	}
	/** Returns i-th argument and increment i. */
	Term const& get_arg( uint& i ) const&;
	/** Asserts i is the number of arguments. */
	bool gets_end( uint i ) const& {
		return i == args().size();
	}
	void get_end( uint i ) const&;
	template<typename Fn>
	Term<std::invoke_result_t<Fn, F const&>> map( Fn&& f ) const {
		Term<std::invoke_result_t<Fn, F const&>> ret = f(fun());
		for( auto& arg : _ref->second ) {
			ret._ref->second.push_back(arg.map(f));
		}
		return ret;
	}
	void iter( std::function<void(Term const&)> const& f ) const& {
		for( auto const& arg : args() ) {
			arg.iter(f);
		}
		f(*this);
	}
	Term const& at( Pos const& pos ) const&;
	Term& operator=( Term && other ) & {
		_ref = std::move(other._ref);
		return *this;
	}
	Term& operator=( Term const& other ) & {
		_ref = other._ref;
		return *this;
	}
	bool operator==( Term const& other ) const {
		return _ref == other._ref;
	}
	bool subterm_eq( Term const& other ) const {
		auto const& [f,ss] = **this;
		auto const& [g,ts] = *other;
		size_t i = 0;
		size_t n = ts.size();
		if( f == g && ss.size() == n ) {
			for( ;; i++ ) {
				if( i == n ) return true;
				if( ss[i] != ts[i] ) break;
			}
		}
		for( ;; i++ ) {
			if( i == n ) return false;
			if( subterm_eq(ts[i]) ) return true;
		}
	}
	bool subterm( Term const& other ) const {
		for( auto const& t : other.args() ) {
			if( subterm_eq(t) ) return true;
		}
		return false;
	}
	Opt<F const&> is_unapplied() const& {
		if( args().empty() ) return {fun()};
		return {};
	}
	F const& as_unapplied() const&;
};
template<typename F>
std::compare_three_way_result_t<F>
operator<=>( Term<F> const& l, Term<F> const& r ) {
	auto root = l.fun() <=> r.fun();
	if( root != 0 ) return root;
	return l.args() <=> r.args();
}
template<typename F, typename S>
requires std::is_constructible_v<F,S>
static Term<F> app( S&& fun, std::vector<Term<F>>&& args ) {
	return Term(std::in_place,std::forward<S>(fun),std::move(args));
}
template<typename F, typename S>
requires std::is_constructible_v<F,S>
static Term<F> app( S&& fun, std::vector<Term<F>>const& args ) {
	return Term(std::in_place,std::forward<S>(fun),args);
}

struct Error : std::exception, Term<std::string> {
	using Term<std::string>::Term;
};
template<typename F>
Term<F> const& Term<F>::get_arg( uint& i ) const& {
	return gets_arg(i) | [&]{ throw Error("#too-few-args",*this); };
}
template<typename F>
void Term<F>::get_end( uint i ) const& {
	if( !gets_end(i) ) throw Error("#expected-rparen",arg(i));
}

inline ulong as_nat( std::string_view const& str ) {
	return is_nat(str) | [&]{ throw Error("#expected-nat",str); };
}

inline ulong as_nat( std::string_view const& str, std::function<bool(ulong)> const& test ) {
	auto ret = as_nat(str);
	if( !test(ret) ) throw Error("#nat-out-of-range",str);
	return ret;
}

template<typename F>
F const& Term<F>::as_unapplied() const& {
	return is_unapplied() | [&]{ throw Error("#unexpected-argument"); };
}

/** deleted for inefficiency */
Opt<std::string_view> is_key( std::string && ) = delete;
Opt<std::string_view> is_key( Term<std::string> && ) = delete;
inline Opt<std::string_view> is_key( std::string const& str ) {
	if( str.starts_with(':') ) return std::string_view(str).substr(1);
	return {};
}
inline Opt<std::string_view> is_key( Term<std::string> const& x ) {
	return x.is_unapplied() >>= []( auto const& val ){ return is_key(val); };
}
Opt<std::string_view> is_str( std::string && ) = delete;
inline Opt<std::string_view> is_str( std::string const& str ) {
	if( str.starts_with('"') ) return std::string_view(str).substr(1,str.size()-2);
	return {};
}
inline Opt<std::string_view> is_str( Term<std::string> const& x ) {
	return x.is_unapplied() >>= []( auto const& val ){ return is_str(val); };
}
inline Opt<bool> is_bool( std::string const& str ) {
	if( str == "true" ) return {true};
	if( str == "false" ) return {false};
	return {};
}
inline bool as_bool( std::string const& str ) {
	return is_bool(str) | [&]{ throw Error("#malformed-bool",str); };
}

struct Exp : Term<std::string> {
	using Term<std::string>::Term;
	Exp( Term<std::string> const& other ) : Term<std::string>(other) {}
	Exp( Term<std::string> && other ) : Term<std::string>(std::move(other)) {}
	static Exp of( std::string const& );
	Opt<bool> is_bool() const& {
		return is_unapplied() >>= ::is_bool;
	}
	bool as_bool() const& {
		return is_bool() | [&]{ throw Error("#expected-bool",*this); };
	}
	Opt<ulong> is_nat() const& {
		return is_unapplied() >>= ::is_nat;
	}
	ulong as_nat() const& {
		return is_nat() | [&]{ throw Error("#expected-nat",*this); };
	}
	ulong as_nat( std::function<bool(ulong)> const& test ) const& {
		auto ret = as_nat();
		if( test(ret) ) return ret;
		throw Error("#nat-out-of-range",*this);
	}
	class Proc;
	Proc proc() const;
	using KeyValProc = std::function<bool(std::string_view const&,Exp const&)>;
	static KeyValProc const NO_KEY;
	static KeyValProc on_key( std::string&& _key, std::function<void(Exp const&)>&& _proc ) {
		return [key = std::move(_key), proc = std::move(_proc)]( std::string_view const& pkey, Exp const& val ){
			if( pkey == key ) {
				proc(val);
				return true;
			}
			return false;
		};
	}
	struct OnKey {
		std::string&& key;
		KeyValProc operator/( auto&& proc ) && {
			return on_key(std::move(key),std::move(proc));
		}
	};
	struct Ender {};
	static inline Ender const END = {};
	static void test();
};
inline Exp::KeyValProc const Exp::NO_KEY = []( std::string_view const&, Exp const& ){ return false; };

class Exp::Proc {
	friend Exp;
	Exp _exp;
	uint _pos;
	Proc( Exp const& e ) : _exp(e), _pos(0) {}
public:
	Exp exp() && { return _exp; }
	Exp const& exp() const& { return _exp; }
	std::string fun() && { return _exp.fun(); }
	std::string const& fun() const& { return _exp.fun(); }
	/** Processes key-value pairs from the ith argument. */
	void operator,( KeyValProc const& proc ) &;
	void operator,( Ender const& ) const {
		if( _exp.args().size() != _pos ) throw Error("#exp-syntax","\"expected ')'\"",":next",_exp.arg(_pos));
	}
	Proc& operator,( std::function<void(Exp const&)>&& arg ) & {
		std::move(arg)(get_arg());
		return *this;
	}

	Opt<Exp> gets_arg() & {
		Opt<Exp> ret;
		if( _pos < _exp.args().size() ) {
			ret = {_exp.arg(_pos)};
			_pos++;
		}
		return ret;
	}
	Exp get_arg() & {
		return gets_arg() | []{ throw Error("#exp-syntax","\"unexpected ')'\""); };
	}
};

inline Exp::Proc Exp::proc() const {
	return Proc(*this);
}

class Reader {
	std::unique_ptr<std::istream> _ptr;
	std::istream& _is;
	class LPar {};
	class RPar {};
	class None {};
	struct Key { std::string str; };
	struct Sym { std::string str; };
	struct Str { std::string str; };
	Sum<None,LPar,RPar,Key,Str,Sym> _fetched;
	void _fetch();
	unsigned int _line = 1;
	unsigned int _column = 1;
	unsigned int _fetched_column = 1;
	std::string _string_fetched() const& {
		if( _fetched.ref<LPar>() ) return "#lparen";
		if( _fetched.ref<RPar>() ) return "#rparen";
		if( auto key = _fetched.ref<Key>() ) return key->str;
		if( auto str = _fetched.ref<Str>() ) return str->str;
		if( auto sym = _fetched.ref<Sym>() ) return sym->str;
		return "#none";
	}
	std::string _read_string_literal() &;
	std::string _read_bar_rest() &;
public:
	Reader( std::istream& is ) : _is(is), _fetched(None()) {}
	Reader( char const* && ) = delete;
	Reader( char const* const& str ) : _ptr(std::make_unique<std::istringstream>(str)), _is(*_ptr), _fetched(None()) {}
	Reader( std::string&& ) = delete;
	Reader( std::string const& str ) : _ptr(std::make_unique<std::istringstream>(str)), _is(*_ptr), _fetched(None()) {}
	template<typename... Args>
	Error error( Args const&... msg ) const& {
		return Error(msg...,":line",std::to_string(_line),":column",std::to_string(_column),":next",_string_fetched());
	}
	bool opens() {
		_fetch();
		if( _fetched.ref<LPar>() ) {
			_fetched = None();
			return true;
		}
		return false;
	}
	void open() {
		if( !opens() ) throw error("#missing-left-paren");
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
		if( !closes() ) throw error("#missing-right-paren");
	}
	Opt<std::string> reads_key() {
		_fetch();
		return _fetched.ref<Key>() >>= [&]( Key key ){
			_fetched = None();
			return Opt{std::move(key.str)};
		};
	}
	Opt<std::string> reads_str() {
		_fetch();
		return _fetched.ref<Str>() >>= [&]( Str str ){
			_fetched = None();
			return Opt{std::move(str.str)};
		};
	}
	Opt<std::string> reads_sym() {
		_fetch();
		return _fetched.ref<Sym>() >>= [&]( Sym sym ){
			_fetched = None();
			return Opt{sym.str};
		};
	}
	std::string read_sym() {
		return reads_sym() | [&]{ throw error("#missing-symbol"); };
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
		if( !reads_sym(str) ) throw error("#missing-symbol",str);
	}
	template<typename F> requires std::invocable<F,std::string const&>
	std::invoke_result_t<F,std::string const&> reads_sym( F&& f ) {
		_fetch();
		if( auto sym = _fetched.ref<Sym>() ) {
			if( auto ret = std::forward<F>(f)(sym->str) ) {
				_fetched = None();
				return ret;
			}
		}
		return {};
	}
	Opt<ulong> reads_nat() {
		return reads_sym( is_nat );
	}
	ulong read_nat() {
		return reads_nat() | [&]{ throw error("#missing-number"); };
	}
	Opt<ulong> reads_nat( std::function<bool(ulong)> const& test ) {
		return reads_nat() >>= [&]( auto n ){
			return test(n) ? Opt{n} : throw error("#invalid-value",std::to_string(n));
		};
	}
	ulong read_nat( std::function<bool(ulong)> const& test ) {
		auto ret = read_nat();
		return test(ret) ? ret : throw error("#invalid-value",std::to_string(ret));
	}
	Opt<bool> reads_bool();
	bool read_bool() {
		return reads_bool() | [&]{ throw error("#invalid-bool"); };
	}
	Opt<long> reads_int() {
		return reads_sym( is_int );
	}
	long read_int() {
		return reads_int() | [&]{ throw error("#invalid-int"); };
	}
	Opt<Exp> reads_exp();
	Exp read_exp() {
		return reads_exp() | [&]{ throw error("#missing-expression"); };
	}
	bool eof() {
		_fetch();
		return (bool)_fetched.ref<None>();
	}
};

template<typename F>
Term<F> const& Term<F>::at( Pos const& pos ) const& {
	Term const* ptr = this;
	for( unsigned char i : pos ) {
		if( ptr->args().size() <= i ) throw Error("#bad-term-position");
		ptr = &ptr->args()[i];
	}
	return *ptr;
}

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
