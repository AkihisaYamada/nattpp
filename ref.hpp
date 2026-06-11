#ifndef _REF_HPP
#define _REF_HPP

#include<memory>
#include"opt.hpp"

/**
 * @brief Non-null shared pointer.
 * 
 * @tparam T the type of the content.
 */
template<typename T, bool _nullable = false>
class Ref {
	std::shared_ptr<T> _ptr;
	T& operator*() && = delete;
	T* operator->() && = delete;
	Ref( std::shared_ptr<T> const& ptr ) : _ptr(ptr) {}
	template<typename S, bool n>
	friend class Ref;
public:
	/**
	 * @brief Null reference
	 */
	Ref() requires _nullable = default;
	/**
	 * @brief Non-null reference can be considered nullable
	 */
	Ref( Ref<T,false> const& org ) requires _nullable : _ptr(org._ptr) {}
	Ref( Ref&& org ) = default;
	Ref( Ref const& org ) = default;
	/** @brief Do not turn a constructed object into reference.
	 * It would need a copy / move. Use Ref<T>::make( Args... ) instead.
	 */
	Ref(T) = delete;
	operator bool() const requires _nullable {
		return (bool)_ptr;
	}
	Ref& operator=( Ref const& other ) = default;
	T& operator*() const & {
		return *_ptr;
	}
	T* operator->() const & {
		return _ptr.get();
	}
	operator Ref<T const>() const {
		return Ref<T const>(_ptr);
	};
	/**
	 * @brief Make the object unique
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
	static Ref make(Ts const&... args) {
		return Ref(std::make_shared<T>(args...));
	}
	template<typename S, bool n1, bool n2>
	friend bool operator==(Ref<S,n1> const& l, Ref<S,n2> const& r);
};

template<typename T>
using OptRef = Ref<T,true>;

template<typename T, bool n1, bool n2>
bool operator==(Ref<T,n1> const& l, Ref<T,n2> const& r) {
	return l._ptr == r._ptr || *l._ptr == *r._ptr;
};

/**
 * @brief Memoized object. Modification to the object will not affect other references.
 * 
 * @tparam T 
 */
template<class T, bool _nullable = false>
class Mem {
	mutable std::shared_ptr<T> _ptr;
	T operator*() && = delete;
	T* operator->() && = delete;
	void _fork() const {
		if( !_ptr.unique() ) {
			_ptr = std::make_shared<T>(*_ptr);
		}
	}
	template<typename S, bool n>
	friend class Mem;
public:
	/**
	 * @brief Null object
	 */
	Mem() requires _nullable = default;
	/**
	 * @brief Non-null object can be considered nullable
	 */
	Mem( Mem<T,false> const& org ) requires _nullable : _ptr(org._ptr) {}
	Mem( Mem const& other ) : _ptr(other._ptr) {}
	Mem( Mem && other ) : _ptr(std::move(other._ptr)) {}
	Mem& operator=( Mem const& other ) & {
		_ptr = other._ptr;
		return *this;
	}
	Mem& operator=( Mem && other ) & {
		_ptr = std::move(other._ptr);
		return *this;
	}
	template<typename... Ts>
	explicit Mem(Ts const&... args) : _ptr(std::make_shared<T>(args...)) {}
	/**
	 * @brief Optional non-null object can be seen as a nullable object
	 */
	template<typename S> requires _nullable && std::is_convertible_v<S,Mem<T>>
	explicit Mem( Opt<S> const& org ) : _ptr( org ? ((Mem<T>)*org)._ptr : nullptr ) {}
	/**
	 * @brief Optional non-null object can be seen as a nullable object
	 */
	template<typename S> requires _nullable && std::is_convertible_v<S,Mem<T>>
	explicit Mem( Opt<S const&> const& org ) : _ptr( org ? ((Mem<T>)*org)._ptr : nullptr ) {}
	operator bool() const requires _nullable {
		return (bool)_ptr;
	}
	/**
	 * @brief Const reference.
	 */
	T const& operator*() const & {
		return *_ptr;
	}
	T const* operator->() const & {
		return _ptr.get();
	}
	/**
	 * @brief Modifiable reference. This will be the unique owner of the object.
	 */
	T& operator*() & {
		_fork();
		return *_ptr;
	}
	T* operator->() & {
		_fork();
		return _ptr.get();
	}
	template<bool n>
	bool operator==( Mem<T,n> const& r ) const {
		return _ptr == r._ptr || *_ptr == *r;
	}
};

template<typename T>
using OptMem = Mem<T,true>;

#endif