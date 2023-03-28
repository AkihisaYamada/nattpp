#ifndef _REF_HPP
#define _REF_HPP

#include<memory>
/**
 * @brief Non-null shared pointer.
 * 
 * @tparam T the type of the content.
 */
template<typename T, bool nullable = false>
class Ref {
	std::shared_ptr<T> _ptr;
	T& operator*() && = delete;
	T* operator->() && = delete;
	Ref( std::shared_ptr<T> const& ptr ) : _ptr(ptr) {}
	template<typename S, bool n>
	friend class Ref;
public:
	Ref( nullptr_t const& n = nullptr ) requires nullable {}
	Ref( Ref const& org ) = default;
	~Ref() = default;
	operator bool() const requires nullable {
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
	static Ref make(Ts... args...) {
		return Ref(std::make_shared<T>(args...));
	}
	template<typename S, bool n1, bool n2>
	friend bool operator==(Ref<S,n1> const& l, Ref<S,n2> const& r);
};

template<typename T>
using OptRef = Ref<T,true>;

template<typename T, bool n1, bool n2>
bool operator==(Ref<T,n1> const& l, Ref<T,n2> const& r) {
	return l._ptr == r._ptr;
};

/**
 * @brief Memoized object. Modification to the object will not affect other references.
 * 
 * @tparam T 
 */
template<class T>
class Mem {
	std::shared_ptr<T> _ptr;
	T operator*() && = delete;
	T* operator->() && = delete;
	Mem( std::shared_ptr<T> const& ptr ) : _ptr(ptr) {}
	void _fork() {
		if( !_ptr.unique() ) {
			_ptr = std::make_shared<T>(*_ptr);
		}
	}
public:
	Mem( Mem const& other ) = default;
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
	/**
	 * @brief Constructing a shared object.
	 * 
	 * @param args arguments to the object constructor
	 * @return a non-null pointer to the constructed object
	 */
	template<typename... Ts>
	static Mem<T> make(Ts... args...) {
		return Mem(std::make_shared<T>(args...));
	}
	template<class S>
	friend bool operator==(Mem<S> const& l, Mem<S> const& r);
};

template<class T>
bool operator==(Mem<T> const& l, Mem<T> const& r) {
	return l._ptr == r._ptr || *l == *r;
}

#endif