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
	Map() {}
	Map( std::initializer_list<Value> list ) : _map(list) {}
	M::iterator begin() { return _map.begin(); }
	M::iterator end() { return _map.end(); }
	M::const_iterator begin() const { return _map.begin(); }
	M::const_iterator end() const { return _map.end(); }
	/**
	 * @brief emplaces a key-value pair.
	 * @return a reference to existing value if the key already exists
	 */
	template<typename... Ts>
	Opt<T&> insert( Ts&&... args ) {
		auto [it,f] = _map.emplace(std::forward<Ts>(args)...);
		if( f ) {
			return {};
		}
		return it->second;
	}
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
