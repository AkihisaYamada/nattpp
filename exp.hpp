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

using Pos = std::vector<unsigned char>;

std::ostream& operator<<( std::ostream& os, Pos const& pos );

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
	Term( S const& fun, Args&&... args ) :
		_mem(App(fun,std::vector<Term>{Term(std::forward<Args>(args))...})) {}
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
	Term& arg( size_t i ) & {
		assert( i < args().size() );
		return args()[i];
	}
	Term const& arg( size_t i ) const& {
		assert( i < args().size() );
		return args()[i];
	}
	template<typename G>
	Term<G> map( std::function<G(F const&)> f ) const {
		Term<G> ret = f(fun());
		for( auto& arg : args() ) {
			ret.args().push_back(arg.map(f));
		}
		return ret;
	}
	Term const& at( Pos const& pos ) const&;
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
	Opt<F const&> unapplied() const& {
		if( args().empty() ) return {fun()};
		return {};
	}
};
template<typename F>
std::compare_three_way_result_t<F>
operator<=>( Term<F> const& l, Term<F> const& r ) {
	auto root = l.fun() <=> r.fun();
	if( root != 0 ) return root;
	return l.args() <=> r.args();
}

/** deleted for inefficiency */
Opt<std::string_view> is_key( std::string && ) = delete;
Opt<std::string_view> is_key( Term<std::string> && ) = delete;
inline Opt<std::string_view> is_key( std::string const& str ) {
	if( str.starts_with(':') ) return std::string_view(str).substr(1);
	return {};
}
inline Opt<std::string_view> is_key( Term<std::string> const& x ) {
	return x.unapplied() >>= []( auto const& val ){ return is_key(val); };
}
Opt<std::string_view> is_str( std::string && ) = delete;
inline Opt<std::string_view> is_str( std::string const& str ) {
	if( str.starts_with('"') ) return std::string_view(str).substr(1,str.size()-1);
	return {};
}
inline Opt<std::string_view> is_str( Term<std::string> const& x ) {
	return x.unapplied() >>= []( auto const& val ){ return is_str(val); };
}

struct Error : std::exception, Term<std::string> {
	using Term<std::string>::Term;
};


struct Exp : Term<std::string> {
	using Term<std::string>::Term;
	Exp( Term<std::string> const& other ) : Term<std::string>(other) {}
	Exp( Term<std::string> && other ) : Term<std::string>(std::move(other)) {}
	static Exp of( std::string const& );
	/** Returns i-th argument and increment i. */
	Opt<Exp> gets_arg( size_t& i ) const& {
		if( i < args().size() ) {
			auto const& a = arg(i);
			if( !is_key(a) ) {
				i++;
				return {a};
			}
		}
		return {};
	}
	Exp get_arg( size_t& i ) const& {
		return gets_arg(i).value_or_throw(Error("#too-few-args",*this));
	}
	void get_end( size_t i ) const& {
		if( i != args().size() ) throw Error("#expected-rparen",arg(i));
	}
	using KeyValProc = std::function<bool(std::string_view const&,Exp const&)>;
	/** Processes key-value pairs from the ith argument. */
	void process_keys( size_t& i, KeyValProc const& f ) const&;
	static void test();
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

class Reader {
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
public:
	Reader( std::istream& is ) : _is(is), _fetched(None()) {}
	template<typename... Args>
	Error error( Args const&... msg ) const& {
		return Error(msg...,":line",std::to_string(_line),":column",std::to_string(_column),":encount",_string_fetched());
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
		return _fetched.ref<Key>() >>= [&]( Key key )->Opt<std::string>{
			_fetched = None();
			return {std::move(key.str)};
		};
	}
	Opt<std::string> reads_str() {
		_fetch();
		return _fetched.ref<Key>() >>= [&]( Key key )->Opt<std::string>{
			_fetched = None();
			return {std::move(key.str)};
		};
	}
	Opt<std::string> reads_sym() {
		_fetch();
		return _fetched.ref<Sym>() >>= [&]( Sym sym )->Opt<std::string>{
			_fetched = None();
			return std::move(sym.str);
		};
	}
	std::string read_sym() {
		return reads_sym().value_or_throw(error("#missing-symbol"));
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
	Opt<unsigned int> reads_nat( std::function<bool(unsigned int)> const& test ) {
		if( auto n = reads_nat() ) {
			if( !test(*n) ) throw error("#invalid-value",std::to_string(*n));
			return n;
		}
	}
	void read_sym( char const* str ) {
		if( !reads_sym(str) ) throw error("#missing-symbol",str);
	}
	unsigned int read_nat() {
		return reads_nat().value_or_throw(error("#missing-number"));
	}
	unsigned int read_nat( std::function<bool(unsigned int)> const& test ) {
		auto ret = read_nat();
		if( !test(ret) ) throw error("#invalid-value",std::to_string(ret));
		return ret;
	}
	int read_int() {
		return std::stoi(read_sym());
	}
	Opt<Exp> reads_exp();
	Exp read_exp() {
		return reads_exp().value_or_throw(error("#missing-expression"));
	}
	bool eof() {
		_fetch();
		return _fetched.ref<None>();
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

template<typename It, typename F>
std::ostream& print_list( std::ostream& os, It it, It const& end, F const& f ) {
	if( it == end ) return os;
	for(;;){
		os << f(*it);
		it++;
		if( it == end ) return os;
		os << ' ';
	}
}

#endif
