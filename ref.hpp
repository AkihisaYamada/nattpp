#ifndef _REF_HPP
#define _REF_HPP

#include<memory>
#include<variant>

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
	Ptr( std::shared_ptr<T>&& ptr ) : _ptr(std::move(ptr)) {}
public:
	Ptr(Ptr const& org) = default;
	Ptr(T const& val) : _ptr(std::make_shared<T>(val)) {}
	~Ptr() = default;
	Ptr& operator=(Ptr const& other) = default;
	T& operator*() const & {
		return *_ptr;
	}
	T* operator->() const & {
		return &*_ptr;
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
 * @brief Memoized object. Modification to the object will not affect other references.
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