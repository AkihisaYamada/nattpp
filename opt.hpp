#ifndef _OPT_HPP
#define _OPT_HPP

#include<optional>
#include<functional>
#include<cassert>

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
	Opt( Opt&& other ) = default;
	Opt( Opt const& other ) = default;
	Opt( T&& org ) : _opt(std::move(org)) {}
	template<typename S> requires std::is_convertible_v<S,T>
	Opt( S const& org ) : _opt(org) {}
	/**
	 * @brief Constructs optional object in-place.
	 */
	template<typename... Ts>
	Opt( std::in_place_t, Ts&&... xs ) : _opt(std::in_place,std::forward<Ts>(xs)...) {}
	operator bool() const {
		return (bool)_opt;
	}
	Opt& operator=( Opt && other ) {
		_opt = std::move(other._opt);
		return *this;
	}
	Opt& operator=( Opt const& other ) {
		_opt = other._opt;
		return *this;
	}
	T const& operator*() const & {
		assert(_opt);
		return *_opt;
	}
	T& operator*() & {
		assert(_opt);
		return *_opt;
	}
	T operator*() && {
		assert(_opt);
		return *std::move(_opt);
	}
	/** @brief Returns a copy of the value or given default. */
	T value_or( T&& def ) {
		if(_opt) return *_opt;
		return def;
	}
	/** @brief Refers to the value or the default. */
	T const& value_or( T const& def ) const & {
		if(_opt) return *_opt;
		return def;
	}
	T const* operator->() const & {
		assert(_opt);
		return _opt.operator->();
	}
	T* operator->() & {
		assert(_opt);
		return _opt.operator->();
	}
	T ref( T const& other ) const& {
		if(_opt) return *_opt;
		return other;
	}
	template<typename O>
	O operator>>=( std::function<O(T&&)> const& f ) && {
		return *this ? f(*_opt) : O();
	}
	template<typename O>
	O operator>>=( std::function<O(T const&)> const& f ) const& {
		return *this ? f(*_opt) : O();
	}
	template<typename U>
	bool contains( U const& other ) const {
		return *this && **this == other;
	}
	template<typename U>
	operator Opt<U>() && = delete;
	template<typename U> requires std::is_convertible_v<T,U>
	operator Opt<U const&>() const& {
		return Opt<U const&>( _opt ? &*_opt : nullptr );
	}
	template<typename U> requires std::is_convertible_v<T,U>
	operator Opt<U&>() & {
		return Opt<U&>( _opt ? &*_opt : nullptr );
	}
	template<class... Args>
	static Opt make( Args&&... args ) {
		return Opt(std::in_place,std::forward<Args>(args)...);
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
	T& operator*() const {
		assert(*this);
		return *_ptr;
	}
	T* operator->() const {
		assert(*this);
		return _ptr;
	}
	bool operator&&( std::function<bool(T const&)> f ) const& {
		return *this && f(*_ptr);
	}
	template<typename U>
	bool contains( U const& other ) const {
		return *this && **this == other;
	}
	/** @brief Returns a copy of the value or given default. */
	T value_or( T&& def ) {
		if(_ptr) return *_ptr;
		return def;
	}
	/** @brief Refers to the value or the default. */
	T const& value_or( T const& def ) const & {
		if(_ptr) return *_ptr;
		return def;
	}
};

#endif