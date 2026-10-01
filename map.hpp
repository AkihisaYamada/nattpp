#ifndef _MAP_HPP
#define _MAP_HPP

#include<map>
#include<unordered_map>
#include"opt.hpp"

template<typename K, typename T>
class Map : std::unordered_map<K,T> {
	using M = std::unordered_map<K,T>;
public:
	using typename M::key_type, typename M::value_type, typename M::iterator, typename M::const_iterator;
	using M::M, M::begin, M::end, M::size, M::empty, M::erase, M::extract, M::clear;
	/**
	 * @brief emplaces a key-value pair.
	 */
	template<typename... Ts>
		requires std::is_constructible_v<value_type,Ts&&...>
	std::pair<T&,bool> emplace( Ts&&... args ) {
		auto [it,f] = M::emplace(std::forward<Ts>(args)...);
		return {it->second,f};
	}
	template<typename L>
	Opt<T&> find( L&& k ) & {
		if( auto it = M::find(std::forward<L>(k)); it != end() ) {
			return it->second;
		}
		return {};
	}
	template<typename L>
	Opt<T const&> find( L&& k ) const & {
		if( auto it = M::find(std::forward<L>(k)); it != end() ) {
			return it->second;
		}
		return {};
	}
	Opt<T&> find( K const& k ) & { return find<K const&>(k); }
	Opt<T const&> find( K const& k ) const & { return find<K const&>(k); }
	void merge(Map& other) { M::merge(static_cast<M&>(other)); }
	void merge(Map&& other) { M::merge(static_cast<M&&>(other)); }
};

template<typename K, typename T>
class OrdMap : std::map<K,T,std::less<>> {
	using M = std::map<K,T,std::less<>>;
public:
	using typename M::value_type, typename M::iterator, typename M::const_iterator;
	using M::M, M::begin, M::end, M::size, M::empty, M::erase;
	/**
	 * @brief emplaces a key-value pair.
	 */
	template<typename... Ts>
		requires std::is_constructible_v<value_type,Ts&&...>
	std::pair<T&,bool> emplace( Ts&&... args ) {
		auto [it,f] = M::emplace(std::forward<Ts>(args)...);
		return {it->second,f};
	}
	template<typename L>
	Opt<T&> find( L&& k ) & {
		if( auto it = M::find(std::forward<L>(k)); it != end() ) {
			return it->second;
		}
		return {};
	}
	template<typename L>
	Opt<T const&> find( L&& k ) const & {
		if( auto it = M::find(std::forward<L>(k)); it != end() ) {
			return it->second;
		}
		return {};
	}
	Opt<T&> find( K const& k ) & { return find<K const&>(k); }
	Opt<T const&> find( K const& k ) const & { return find<K const&>(k); }
};

#endif
