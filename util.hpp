#ifndef _UTIL_HPP
#define _UTIL_HPP

#include<map>
#include<set>
#include<functional>
#include<string>
#include"opt.hpp"

#define DEB(a) do { std::cerr << __FILE__ << ':' << __LINE__ << ' ' << a << std::endl; } while(0)
#define DEBval(a) ([&]{ auto const& _r = a; DEB(_r); return _r; }())

static Opt<int> to_int( std::string const& str ) try {
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
template<typename... Args>
std::function<bool(Args...)> operator||(std::function<bool(Args...)>&& f, std::function<bool(Args...)>&& g) {
	return [f = std::move(f), g = std::move(g)](Args... xs) {
		return f(xs...) || g(xs...);
	};
}
template<typename... Args>
std::function<bool(Args...)> operator||(std::function<bool(Args...)>const& f, std::function<bool(Args...)>const& g) {
	return [&](Args... xs) {
		return f(xs...) || g(xs...);
	};
}
#endif