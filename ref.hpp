#ifndef _REF_HPP
#define _REF_HPP

#include<memory>
#include<ostream>

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
			_ptr = new T(*_ptr);
		}
	}
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
 * @brief Non-null, modifiable object.
 * 
 * @tparam T 
 */
template<class T>
class Safe {
	Ptr<T> _ptr;
public:
	template<typename... Ts>
	Safe(Ts... args...) : _ptr(new T(args...)) {}
	Safe(Safe const& other) = default;
	T const& operator*() const {
		return *_ptr;
	}
	T const* operator->() const {
		return _ptr.operator->();
	}
	/**
	 * @brief Modifiable reference. It will ensure the content is unique.
	 */
	T& operator*() {
		_ptr.fork();
		return *_ptr();
	}
	T* operator->() {
		_ptr.fork();
		return _ptr.operator->();
	}
	template<class S>
	friend bool operator==(Safe<S> const& l, Safe<S> const& r);
};

template<class T>
bool operator==(Safe<T> const& l, Safe<T> const& r) {
	return l._ptr == r._ptr || *l == *r;
};

#endif