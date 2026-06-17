#ifndef SET_HPP
#define SET_HPP

#include<set>
#include"opt.hpp"

template<typename T>
class Set {
	using _Base = std::set<T,std::less<>>;
	_Base _set;
public:
	template<typename... Args>
		requires std::is_constructible_v<_Base,Args...>
	Set( Args&&... args ) : _set( std::forward<Args>(args)... ) {}
	Set( std::initializer_list<T> args ) : _set(args) {}
	Opt<T const&> find( T const& x ) const& {
		if( auto const& it = _set.find(x); it != _set.end() ) {
			return {*it};
		}
		return {};
	}
	bool erase( T const& x ) {
		return _set.erase(x);
	}
};

#endif