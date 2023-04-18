#ifndef _MAP_HPP
#define _MAP_HPP

#include<map>
#include"opt.hpp"

template<typename K, typename T>
class Map {
	typedef std::map<K,T,std::less<>> M;
	M _map;
public:
	typedef M::value_type Value;
	Map( std::initializer_list<Value> list ) : _map(list) {}
	M::iterator begin() { return _map.begin(); }
	M::iterator end() { return _map.end(); }
	M::const_iterator begin() const { return _map.begin(); }
	M::const_iterator end() const { return _map.end(); }
	template<typename L>
	Opt<T&> find( L const& k ) & {
		auto it = _map.find(k);
		if( it == _map.end() ) {
			return {};
		}
		return it->second;
	}
	template<typename L>
	Opt<T const&> find( L const& k ) const & {
		auto it = _map.find(k);
		if( it == _map.end() ) {
			return {};
		}
		return it->second;
	}
};

#endif
