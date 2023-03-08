#ifndef _EXP_HPP
#define _EXP_HPP

#include<cassert>
#include<string>
#include<vector>
#include<set>
#include<optional>
#include<functional>
#include<iostream>
#include<exception>
#include<variant>

#include"string.hpp"

class Exp {
	std::variant<String,Ptr<std::vector<Exp>const>> _un;
	/**
	 * @brief optional reference.
	 * 
	 */
	template<typename T>
	class OptRef {
		T* ptr;
	public:
		OptRef(T* ptr) : ptr(ptr) {}
		T& operator*() const { return *ptr; }
		T* operator->() const { return ptr; }
		operator bool() const { return ptr; }
	};
public:
	struct ParseError : std::exception {
		std::string const& str;
		ParseError(std::string const&& str) : str(str) {}
	};
	struct ExpectsSym : std::exception {
	};
	struct ExpectsApp : std::exception {
	};
	Exp( String const& str ) : _un(str) {}
	Exp( std::vector<Exp> const&& vec ) : _un(vec) {}
	Exp( std::initializer_list<Exp> list ) : _un(std::vector<Exp>(list)) {}
	OptRef<String const> sym() const {
		return OptRef(std::get_if<String>(&_un));
	}
	OptRef<std::vector<Exp> const> app() const {
		auto const& p = std::get_if<Ptr<std::vector<Exp> const>>(&_un);
		return OptRef( p ? &**p : nullptr );
	}
};
inline bool operator==( Exp const& l, std::string_view r ) {
	auto sym = l.sym();
	return sym && *sym == r;
}
class Dict {
	std::set<String,std::less<>> _set;
public:
	String const& touch( String const& str ) {
		return *_set.insert(str).first;
	}
	String const& touch( std::string const& str ) {
		return *_set.insert(str).first;
	}
	String const& touch( char const* str ) {
		return *_set.insert(str).first;
	}

	std::optional<Exp> reads_exp(std::istream& is);
};

std::ostream& operator<<(std::ostream& os, Exp const& e);

#endif
