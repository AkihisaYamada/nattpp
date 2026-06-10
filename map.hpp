#ifndef _MAP_HPP
#define _MAP_HPP

#include<map>
#include"opt.hpp"

template<typename K, typename T>
class Map : std::map<K,T,std::less<>> {
	typedef std::map<K,T,std::less<>> M;
public:
	using typename M::value_type, typename M::iterator, typename M::const_iterator;
	using M::M, M::begin, M::end, M::size, M::empty, M::erase;
	/**
	 * @brief emplaces a key-value pair.
	 * @return a reference to existing value if the key already exists
	 */
	template<typename... Ts>
	Opt<T&> insert( Ts&&... args ) {
		auto [it,f] = M::emplace(std::forward<Ts>(args)...);
		if( f ) {
			return {};
		}
		return it->second;
	}
	template<typename L>
	Opt<T&> find( L const& k ) & {
		auto it = M::find(k);
		if( it == end() ) {
			return {};
		}
		return it->second;
	}
	Opt<T&> find( K const& k ) & {
		return find<K>(k);
	}
	template<typename L>
	Opt<T const&> find( L const& k ) const & {
		auto it = M::find(k);
		if( it == end() ) {
			return {};
		}
		return it->second;
	}
	Opt<T const&> find( K const& k ) const & {
		return find<K>(k);
	}
};

#endif
