#ifndef _REF_HPP
#define _REF_HPP

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
 * @brief Reference counter.
 * 
 * @tparam T the type of the content.
 */
template<typename T>
class Ptr {
	struct Body {
		unsigned int nref = 0;
		T body;
		Body() {}
		Body(T const& body) : body(body) {}
		Body(T&& body) : body(std::move(body)) {}
	};
	Body* ptr;
	Ptr(Body* ptr) : ptr(ptr) {}
public:
	Ptr() : ptr(new Body()) {}
	Ptr(T const& body) : ptr(new Body(body)) {}
	Ptr(T&& body) : ptr(new Body(std::move(body))) {}
	Ptr(Ptr const& org) : ptr(org.ptr) {
		ptr->nref++;
	}
	~Ptr() {
		if( ptr->nref == 0 ) {
			delete ptr;
		} else {
			ptr->nref--;
		}
	}
	Ptr& operator=(Ptr const& other) {
		this->~Ptr<T>();
		ptr = other.ptr;
		ptr->nref++;
		return *this;
	}
	T& operator*() const {
		return ptr->body;
	}
	T* operator->() const {
		return &ptr->body;
	}
	/**
	 * @brief forks the referenced object.
	 */
	void fork() {
		if( ptr->nref != 0 ) {
			ptr->nref--;
			ptr = new Body(ptr->body);
		}
	}
	bool last() const {
		return ptr->nref == 0;
	}
	template<typename S>
	friend bool operator==(Ptr<S> const& l, Ptr<S> const& r);
};

template<typename T>
bool operator==(Ptr<T> const& l, Ptr<T> const& r) {
	return l.ptr == r.ptr || *l == *r;
};

template<class T>
class Safe {
	Ptr<T> _ref;
public:
	Safe() : _ref() {}
	Safe(T const& val) : _ref(val) {}
	Safe(T&& val) : _ref(std::move(val)) {}
	T const& operator*() const {
		return *_ref;
	}
	T const* operator->() const {
		return &*_ref;
	}
	T& operator*() {
		_ref.fork();
		return *_ref;
	}
	T* operator->() {
		return &operator*();
	}
	template<class S>
	friend bool operator==(Safe<S> const& l, Safe<S> const& r);
};

template<class T>
bool operator==(Safe<T> const& l, Safe<T> const& r) {
	return l._ref == r._ref;
};

#endif