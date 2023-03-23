#ifndef _REF_HPP
#define _REF_HPP

#include<memory>
#include<variant>
#include<optional>

/**
 * @brief Temporary nullable pointers.
 * An object can only refer to an lvalue, and only accessible in the same scope.
 * Functions returning this type must be sure that the pointed object exists in the scope of the return value.
 * @tparam T 
 */
template<typename T>
class TempOpt {
	T* ptr;
	/**
	 * @brief rvalue cannot be pointed.
	 */
	TempOpt(T&&) = delete;
	/**
	 * @brief Do not substitute, as it may break scope.
	 */
	TempOpt& operator=( TempOpt<T> const& ) = delete;
public:
	TempOpt( std::nullopt_t = std::nullopt ) : ptr(nullptr) {}
	TempOpt( T& l ) : ptr(&l) {}
	operator bool() const { return ptr; }
	T& operator*() const { return *ptr; }
	T* operator->() const { return ptr; }
};

/**
 * @brief Safer access to variant alternatives.
 * Works like std::get_if, but return is not a pointer.
 */
template<typename T, typename... Ts>
constexpr TempOpt<T const> ref_if( std::variant<Ts...> const& un ) noexcept {
	if( auto p = std::get_if<T>(&un) ) {
		return *p;
	}
	return std::nullopt;
}
template<typename T, typename... Ts>
constexpr TempOpt<T> ref_if( std::variant<Ts...>& un ) noexcept {
	if( auto p = std::get_if<T>(&un) ) {
		return *p;
	}
	return std::nullopt;
}
template<typename T, typename... Ts>
constexpr std::optional<T> ref_if( std::variant<Ts...> && un ) noexcept {
	if( auto p = std::get_if<T>(&un) ) {
		return std::move(*p);
	}
	return std::nullopt;
}

/**
 * @brief Non-null shared pointer.
 * 
 * @tparam T the type of the content.
 */
template<typename T>
class Ptr {
	std::shared_ptr<T> _ptr;
	T& operator*() && = delete;
	T* operator->() && = delete;
	Ptr( std::shared_ptr<T> const& ptr ) : _ptr(ptr) {}
public:
	Ptr(Ptr const& org) = default;
	~Ptr() = default;
	Ptr& operator=(Ptr const& other) = default;
	T& operator*() const & {
		return *_ptr;
	}
	T* operator->() const & {
		return _ptr;
	}
	/**
	 * @brief forks the referenced object.
	 */
	void fork() {
		if( !_ptr.unique() ) {
			_ptr = std::make_shared<T>(*_ptr);
		}
	}
	/**
	 * @brief Constructing a shared object.
	 * 
	 * @param args arguments to the object constructor
	 * @return a non-null pointer to the constructed object
	 */
	template<typename... Ts>
	static Ptr<T> make(Ts... args...) {
		return Ptr(std::make_shared<T>(args...));
	}
	template<typename S>
	friend bool operator==(Ptr<S> const& l, Ptr<S> const& r);
};

template<typename T>
bool operator==(Ptr<T> const& l, Ptr<T> const& r) {
	return l._ptr == r._ptr;
};

/**
 * @brief Memoization. Modification to the object is permitted, but other references will not be affected.
 * 
 * @tparam T 
 */
template<class T>
class Mem {
	Ptr<T> _ptr;
	T operator*() && = delete;
	T* operator->() && = delete;
	Mem( Ptr<T> const& ptr ) : _ptr(ptr) {}
public:
	Mem( Mem const& other ) = default;
	/**
	 * @brief Const reference.
	 */
	T const& operator*() const & {
		return *_ptr;
	}
	T const* operator->() const & {
		return _ptr.operator->();
	}
	/**
	 * @brief Modifiable reference. This will be the unique owner of the object.
	 */
	T& operator*() & {
		_ptr.fork();
		return *_ptr;
	}
	T* operator->() & {
		_ptr.fork();
		return _ptr.operator->();
	}
	/**
	 * @brief Constructing a shared object.
	 * 
	 * @param args arguments to the object constructor
	 * @return a non-null pointer to the constructed object
	 */
	template<typename... Ts>
	static Mem<T> make(Ts... args...) {
		return Mem(Ptr<T>::make(args...));
	}
	template<class S>
	friend bool operator==(Mem<S> const& l, Mem<S> const& r);
};

template<class T>
bool operator==(Mem<T> const& l, Mem<T> const& r) {
	return l._ptr == r._ptr || *l == *r;
};

#endif