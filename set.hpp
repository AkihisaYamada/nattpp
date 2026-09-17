#ifndef SET_HPP
#define SET_HPP

#include<set>
#include<unordered_set>
#include"opt.hpp"

template<typename T>
class Set {
	using _Base = std::unordered_set<T>;
	_Base _set;
public:
	template<typename... Args>
		requires std::is_constructible_v<_Base,Args...>
	Set( Args&&... args ) : _set( std::forward<Args>(args)... ) {}
	Set( std::initializer_list<T> args ) : _set(args) {}
	template<typename... Args>
		requires std::is_constructible_v<T,Args...>
	std::pair<T const&, bool> emplace( Args&&... args ) & {
		auto [it,fl] = _set.emplace( std::forward<Args>(args)... );
		return {*it,fl};
	}
	auto begin() & { return _set.begin(); }
	auto begin() const& { return _set.begin(); }
	auto end() const& { return _set.end(); }
	bool empty() const& { return _set.empty(); }
	void clear() & { _set.clear(); }
	Opt<T const&> find( T const& x ) const& {
		if( auto const& it = _set.find(x); it != _set.end() ) {
			return {*it};
		}
		return {};
	}
	bool erase( T const& x ) & {
		return _set.erase(x);
	}
	size_t erase_if( std::function<bool(T const&)> const& test ) {
		return std::erase_if(_set,test);
	}
};

template<typename T>
class OrdSet {
	using _Base = std::set<T,std::less<>>;
	_Base _set;
public:
	template<typename... Args>
		requires std::is_constructible_v<_Base,Args...>
	OrdSet( Args&&... args ) : _set( std::forward<Args>(args)... ) {}
	OrdSet( std::initializer_list<T> args ) : _set(args) {}
	template<typename... Args>
		requires std::is_constructible_v<T,Args...>
	std::pair<T const&, bool> emplace( Args&&... args ) & {
		auto [it,fl] = _set.emplace( std::forward<Args>(args)... );
		return {*it,fl};
	}
	auto begin() & { return _set.begin(); }
	auto begin() const& { return _set.begin(); }
	auto end() const& { return _set.end(); }
	bool empty() const& { return _set.empty(); }
	void clear() & { _set.clear(); }
	Opt<T const&> find( T const& x ) const& {
		if( auto const& it = _set.find(x); it != _set.end() ) {
			return {*it};
		}
		return {};
	}
	bool erase( T const& x ) & {
		return _set.erase(x);
	}
};

#endif