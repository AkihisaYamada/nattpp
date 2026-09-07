#ifndef _OPT_HPP
#define _OPT_HPP

#include<optional>
#include<functional>
#include<cassert>
#include <type_traits>

/**
 * @brief A wrapper for std::optional.
 */
template<typename T>
class Opt {
	std::optional<T> _opt;
	template<typename S>
	friend class Opt;
public:
	Opt() {}
	Opt( Opt&& other ) = default;
	Opt( Opt const& other ) = default;
	Opt( T&& org ) : _opt(std::move(org)) {}
	Opt( T const& org ) : _opt(org) {}
	template<typename S>
		requires std::is_constructible_v<T,S>
	Opt( Opt<S> const& other ) {
		if( other ) _opt = {*other};
	}
	/**
	 * @brief Constructs optional object in-place.
	 */
	template<typename... Ts>
	Opt( std::in_place_t, Ts&&... xs ) : _opt(std::in_place,std::forward<Ts>(xs)...) {}
	explicit operator bool() const {
		return (bool)_opt;
	}
	bool operator!() const {
		return !_opt;
	}
	Opt& operator=( Opt && other ) & {
		_opt = std::move(other._opt);
		return *this;
	}
	Opt& operator=( Opt const& other ) & {
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
	T value_or( T&& def ) && {
		if(_opt) return std::move(*_opt);
		return std::move(def);
	}
	/** @brief Refers to the value or the default. */
	T const& value_or( T const& def ) const & {
		if(_opt) return *_opt;
		return def;
	}
	/** @brief Copies the value or computes default. */
	T operator||( std::function<T()> const& def ) const& {
		if(_opt) return *_opt;
		return def();
	}
	template<typename E>
	T value_or_throw( E const& err ) {
		if(_opt) return *_opt;
		throw err;
	}
	template<typename E>
	T const& value_or_throw( E const& err ) const& {
		if(_opt) return *_opt;
		throw err;
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
	template<typename F>
	auto operator>>=( F const& f ) && {
		using O = std::invoke_result_t<F,T>;
		return *this ? f(std::move(*_opt)) : O{};
	}
	template<typename F>
	auto operator>>=( F const& f ) const& {
		using O = std::invoke_result_t<F, T const&>;
		return *this ? f(*_opt) : O{};
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
		requires std::is_constructible_v<T,Args...>
	static Opt make( Args&&... args ) {
		return Opt(std::in_place,std::forward<Args>(args)...);
	}
	template<class... Args>
		requires std::is_constructible_v<T,Args...>
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
	explicit operator bool() const { return _ptr; }
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
	/** @brief Returns a copy of the value or moves the given default. */
	T value_or( T&& def ) {
		if(_ptr) return *_ptr;
		return std::move(def);
	}
	/** @brief Returns a reference to the value or given default. */
	T const& value_or( T const& def ) {
		if(_ptr) return *_ptr;
		return def;
	}
	/** @brief Copies the value or computes default. */
	T operator||( std::function<T()> const& def ) const& {
		if(_ptr) return *_ptr;
		return def();
	}
	template<typename E>
	T& value_or_throw( E const& err )& {
		if(_ptr) return *_ptr;
		throw err;
	}
	template<typename E>
	T const& value_or_throw( E const& err ) const& {
		if(_ptr) return *_ptr;
		throw err;
	}
	template<typename F>
	auto operator>>=( F const& f ) & {
		using O = std::invoke_result_t<F,T&>;
		return *this ? f(*_ptr) : O{};
	}
	template<typename F>
	auto operator>>=( F const& f ) const& {
		using O = std::invoke_result_t<F, T const&>;
		return *this ? f(*_ptr) : O{};
	}
};

template<typename T, typename U>
bool operator==( Opt<T> const& x, Opt<U> const& y ) {
	return x ? y && *x == *y : !y;
}

#endif