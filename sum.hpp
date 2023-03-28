#ifndef _SUM_HPP
#define _SUM_HPP

#include<variant>
#include "opt.hpp"

/**
 * @brief Wrapper for std::variant.
 * 
 * @tparam Ts 
 */
template<typename... Ts>
class Sum {
	std::variant<Ts...> _un;
public:
	Sum() {}
	template<typename T>
	Sum( T && v ) : _un(std::move(v)) {}
	template<typename T>
	Sum( T const& v ) : _un(v) {}
	template<std::size_t n>
	Opt<std::variant_alternative_t<n,std::variant<Ts...>> const&> ref() const& {
		if( auto p = std::get_if<n>(&_un) ) {
			return *p;
		}
		return {};
	}
	template<std::size_t n>
	Opt<std::variant_alternative_t<n,std::variant<Ts...>>&> ref() & {
		if( auto p = std::get_if<n>(&_un) ) {
			return *p;
		}
		return {};
	}
	template<std::size_t n>
	Opt<std::variant_alternative_t<n,std::variant<Ts...>>> ref() && {
		if( auto p = std::get_if<n>(&_un) ) {
			return std::move(*p);
		}
		return {};
	}
	/**
	 * @brief Optional reference access
	 * 
	 * @tparam T 
	 * @return Opt<T const&> 
	 */
	template<typename T>
	Opt<T const&> ref() const & {
		if( auto p = std::get_if<T>(&_un) ) {
			return *p;
		}
		return {};
	}
	template<typename T>
	Opt<T&> ref() & {
		if( auto p = std::get_if<T>(&_un) ) {
			return *p;
		}
		return {};
	}
	template<typename T>
	Opt<T> ref() && {
		if( auto p = std::get_if<T>(&_un) ) {
			return *p;
		}
		return {};
	}
};

#endif