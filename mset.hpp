#ifndef MSET_HPP
#define MSET_HPP

#include<iostream>
#include"map.hpp"

template<typename T>
struct MSet {
private:
	OrdMap<T,unsigned> _map;
public:
	MSet() {}
	MSet( T const& x ) : _map{{x,1}} {}
	auto begin() && = delete;
	auto begin() & { return _map.begin(); }
	auto begin() const& { return _map.begin(); }
	auto end() const& { return _map.end(); }
	MSet& map_merge(
		MSet&& Y,
		auto const& f
	) & {
		auto xit = begin(), yit = Y.begin();
		for(;;) {
			if( yit == Y.end() ) break;
			if( xit == end() ) {
				_map.merge(std::move(Y._map));
				break;
			}
			if( xit->first < yit->first ) {
				xit++;
			} else {
				auto&& y = Y._map.extract(yit++);
				if( xit->first == y.key() ) {
					f( xit->second, y.mapped() );
					xit++;
				} else {
					xit = _map.insert(xit,std::move(y));
				}
			}
		}
		return *this;
	}

	MSet& operator+=( MSet&& Y ) & {
		return map_merge(std::move(Y),[]( unsigned& x, unsigned y ){ x += y; });
	}
	MSet& join_eq( MSet&& Y ) & {
		return map_merge(std::move(Y),[]( unsigned& x, unsigned y ){ x = std::max(x,y); });
	}
};

template<typename T>
std::ostream& operator<<( std::ostream& os, MSet<T> const& X ) {
	return os << '(' << print_list(X,[]( auto const& xn ){ return Printable([&]( std::ostream& os )->std::ostream&{ return os << '(' << xn.first << ' ' << xn.second << ')'; }); }) << ')';
}

template<typename T>
bool operator>=( MSet<T> const& X, MSet<T> const& Y ) {
	for( auto xit = X.begin(), yit = Y.begin();; xit++ ) {
		if( yit == Y.end() ) return true;
		if( xit == X.end() || xit->first > yit->first ) return false;
		if( xit->first == yit->first ) {
			if( xit->second < yit->second ) return false;
			yit++;
		}
	}
}

template<typename T>
using MSets = std::vector<MSet<T>>;

template<typename T>
std::ostream& operator<<( std::ostream& os, MSets<T> const& Xs ) {
	return os << '(' << print_list(Xs) << ')';
}
template<typename T>
MSets<T> operator+( MSets<T> const& Xs, MSets<T> const& Ys ) {
	MSets<T> ret;
	for( auto const& X : Xs ) {
		for( auto const& Y : Ys ) {
			ret.emplace_back(X) += MSet(Y);
		}
	}
	return std::move(ret);
}

#endif