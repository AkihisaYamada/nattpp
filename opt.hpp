#ifndef _OPT_HPP
#define _OPT_HPP

#include<optional>

/**
 * @brief A wrapper for std::optional.
 */
template<typename T>
class Opt {
	std::optional<T> _opt;
	template<typename U>
	friend class Opt;
public:
	Opt() {}
	Opt( Opt&& other ) : _opt(std::move(other._opt)) {}
	Opt( Opt const& other ) : _opt(other._opt) {}
	Opt( T&& org ) : _opt(std::move(org)) {}
	template<typename S> requires std::is_convertible_v<S,T>
	Opt( S const& org ) : _opt(org) {}
	/**
	 * @brief Constructs optional object in-place.
	 */
	template<typename... Ts>
	Opt( std::in_place_t const& t, Ts&&... xs... ) : _opt(t,std::forward<Ts>(xs)...) {}
	operator bool() const {
		return (bool)_opt;
	}
	T const& operator*() const & {
		return *_opt;
	}
	T& operator*() & {
		return *_opt;
	}
	T operator*() && {
		return *std::move(_opt);
	}
	T const* operator->() const & {
		return _opt.operator->();
	}
	T* operator->() & {
		return _opt.operator->();
	}
	template<class... Args>
	T& emplace( Args&&... args ) & {
		return _opt.emplace(std::forward<Args>(args)...);
	}
};

/**
 * @brief Optional reference.
 * An object can only refer to an lvalue, and only accessible in the same scope.
 * Functions returning this type must be sure that the pointed object exists in the scope of the return value.
 * @tparam T 
 */
template<typename T>
class Opt<T &> {
	T* _ptr;
	Opt( T* ptr ) : _ptr(ptr) {}
	/**
	 * @brief rvalue cannot be pointed.
	 */
	Opt(T&&) = delete;
	/**
	 * @brief Do not substitute, as it may break scope.
	 */
	Opt& operator=( Opt<T> const& ) = delete;
	template<typename S>
	friend class Opt;
public:
	Opt() : _ptr(nullptr) {}
	Opt( T& l ) : _ptr(&l) {}
	operator Opt<T const&>() { return _ptr; }
	operator bool() const { return _ptr; }
	T& operator*() const { return *_ptr; }
	T* operator->() const { return _ptr; }
};

#endif