#ifndef _UTIL_HPP
#define _UTIL_HPP

#include<set>
#include<functional>
#include<string>
#include<iostream>
#include"opt.hpp"

#define DEB(a) do { std::cerr << __FILE__ << ':' << __LINE__ << ' ' << a << std::endl; } while(0)
#define DEBval(a) ([&]{ auto const& _r = a; DEB(_r); return _r; }())

#define ASSERTED(p) ([&](auto&& x)->decltype(auto){ assert(x); return static_cast<decltype(x)>(x); }(p))

/** binary eta-expansion, to avoid the crazy C++ syntax... */
#define BINARY(f) ([](auto&& x, auto&& y) -> decltype(f(std::forward<decltype(x)>(x),std::forward<decltype(y)>(y))) {\
    return f(std::forward<decltype(x)>(x),std::forward<decltype(y)>(y));\
})

enum { NONE = 0, RULE = 1 << 1, PAIR = 1 << 2, USE = 1 << 3, INIT = 1 << 4, DEBUG = 1 << 5 };

std::string escape( std::string const& str );

template<typename I, typename E, typename F>
bool for_all( I it, E const& end, F const& f ) {
	for( ; it != end; it++ ) {
		if( !f(*it) ) return false;
	}
	return true;
}
template<typename C, typename F>
bool for_all( C const& c, F const& f ){
	return chain(c.begin(),c.end(),f);
}

template<typename F, typename T, typename I, typename E>
std::remove_cvref_t<T> chain( T&& x, F const& f, I it, E const& end ) {
	std::remove_cvref_t<T> ret = std::forward<T>(x);
	for( ; it != end; it++ ) {
		f(ret,*it);
	}
	return std::move(ret);
}
template<typename F, typename T, typename C>
std::remove_cvref_t<T> chain( T&& x, F const& f, C const& c ){
	return chain(std::forward<T>(x),f,c.begin(),c.end());
}
template<typename T>
T sum( std::vector<T> const& args ) {
	return chain(T(0),BINARY(operator+=),args);
}
template<typename T>
T prod( std::vector<T> const& args ) {
	return chain(T(1),BINARY(operator*=),args);
}

static Opt<int> is_int( std::string const& str ) try {
	return std::stoi(str);
} catch( std::exception const& err ) {
	return {};
}

template<typename T>
auto operator<=>( std::multiset<T> const& l, std::multiset<T> const& r ) {
	return lexicographical_compare_three_way(l.begin(),l.end(),r.begin(),r.end());
}

template<typename M>
	using IteratorFor = std::conditional<std::is_const_v<M>,typename M::const_iterator,typename M::iterator>::type;

template<typename M1, typename M2>
	requires (std::input_iterator<typename M1::iterator> &&
		std::input_iterator<typename M2::iterator>)
void iter2(
	M1& m1,
	M2& m2,
	std::function<void(IteratorFor<M1>&, IteratorFor<M2>&)> f12,
	std::function<void(IteratorFor<M1>&)> f1,
	std::function<void(IteratorFor<M2>&)> f2
) {
	auto it1 = m1.begin();
	auto end1 = m1.end();
	auto it2 = m2.begin();
	auto end2 = m2.end();
	auto ends2 = [&](){
		if( it2 == end2 ) {
			do {
				f1(it1);
				it1++;
			}
			while( it1 != end1 );
			return true;
		}
		return false;
	};
	for(;;) {
		if( it1 == end1 ) {
			for( ; it2 != end2; it2++ ) {
				f2(it2);
			}
			return;
		}
		if( ends2() ) {
			return;
		}
		while( it1->first != it2->first ) {
			if( it1->first > it2->first ) {
				f2(it2);
				it2++;
				if( ends2() ) {
					return;
				}
			} else {
				f1(it1);
				it1++;
				if( it1 == end1 ) {
					do {
						f2(it2);
						it2++;
					} while( it2 != end2 );
					return;
				}
			}
		}
		f12(it1,it2);
		it1++; it2++;
	}
}

/** pointwise disjunction */
template<typename... Args>
std::function<bool(Args...)> operator||(std::function<bool(Args...)>&& f, std::function<bool(Args...)>&& g) {
	return [f = std::move(f), g = std::move(g)](Args... xs) {
		return f(xs...) || g(xs...);
	};
}
/** pointwise disjunction */
template<typename... Args>
std::function<bool(Args...)> operator||(std::function<bool(Args...)>const& f, std::function<bool(Args...)>const& g) {
	return [&](Args... xs) {
		return f(xs...) || g(xs...);
	};
}

struct Printable {
	std::function<std::ostream&(std::ostream&)> const print;
	Printable( std::function<std::ostream&(std::ostream&)>&& f ) : print(std::move(f)) {}
	Printable( Printable&& ) = default;
	Printable( Printable const& ) = delete;
	Printable& operator=( Printable&& ) & = delete;
	Printable& operator=( Printable const& ) & = delete;
};

inline std::ostream& operator<<( std::ostream& os, Printable const& p ) {
	return p.print(os);
}

template<typename I, typename E, typename F>
Printable print_list( I&& begin, E&& end, F && f ) {
	return {[
		begin = std::forward<I>(begin),
		end = std::forward<E>(end),
		f = std::forward<F>(f)
	]( std::ostream& os ) -> std::ostream& {
		auto it = std::move(begin);
		if( it == end ) return os;
		for(;;) {
			os << f(it);
			it++;
			if( it == end ) return os;
			os << ' ';
		}
	}};
}
template<typename C, typename F>
Printable print_list( C const& c, F const& f ) {
	return print_list( c.begin(), c.end(), [&]( auto const& it ){ return f(*it); } );
}
template<typename C>
Printable print_list( C const& c ) {
	return print_list( c.begin(), c.end(), [&]( auto const& it ){ return *it; } );
}

template<typename T1, typename T2>
std::ostream& operator<<( std::ostream& os, std::pair<T1,T2> const& pair ) {
	return os << "{" << pair.first << ", " << pair.second << "}";
}

#endif