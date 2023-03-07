#ifndef _REF_HPP
#define _REF_HPP

#include<ostream>

template<typename T>
class Ref {
	struct Body {
		unsigned int nref;
		T body;
		Body() : nref(0) {}
		Body(T const& body) : nref(0), body(body) {}
		Body(T&& body) : nref(0), body(body) {}
	};
	Body* ptr;
	Ref(Body* ptr) : ptr(ptr) {}
public:
	Ref() : ptr(new Body()) {}
	Ref(T const& body) : ptr(new Body(body)) {}
	Ref(T&& body) : ptr(new Body(body)) {}
	Ref(Ref const& org) : ptr(org.ptr) {
		ptr->nref++;
	}
	~Ref() {
		if( ptr->nref == 0 ) {
			delete ptr;
		} else {
			ptr->nref--;
		}
	}
	Ref& operator=(Ref const& other) {
		this->~Ref<T>();
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
	 * 
	 * @return Ref& 
	 */
	Ref& fork() {
		if( ptr->nref == 0 ) {// not shared, one can modify the object
			return *this;
		}
		ptr->nref--;
		ptr = new Body(ptr->body);
		return *this;
	}
	template<typename S>
	friend bool operator==(Ref<S> const& l, Ref<S> const& r);
};

template<typename T>
bool operator==(Ref<T> const& l, Ref<T> const& r) {
	return l.ptr == r.ptr || *l == *r;
};

template<class T>
class Safe {
	Ref<T> _ref;
public:
	Safe(T&& body) : _ref(body) {}
	operator T const& () const {
		return *_ref;
	}
	operator T& () {
		_ref.fork();
		return *_ref;
	}
	template<class S>
	friend bool operator==(Safe<S> const& l, Safe<S> const& r);
};

template<class T>
bool operator==(Safe<T> const& l, Safe<T> const& r) {
	return l._ref == r._ref || (T const&)l == r;
};
template<class T>
bool operator<(Safe<T> const& l, Safe<T> const& r) {
	return (T const&)l < r;
};
template<class T>
bool operator<(Safe<T> const& l, T const& r) {
	return l < r;
};
template<class T>
bool operator<(T const& l, Safe<T> const& r) {
	return l < r;
};
template<class T>
inline std::ostream& operator<<(std::ostream& os, Safe<T> const& x) {
	return os << (T const&)x;
}

#endif