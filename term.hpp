#ifndef TERM_HPP_
#define TERM_HPP_
#include<map>
#include<istream>
#include "exp.hpp"

class FunInfo {
	unsigned char _arity;
public:
	FunInfo() {}
	void set_arity( unsigned char arity ) { _arity = arity; }
	unsigned char arity() const { return _arity; }
};
class Sig {
	std::map<String,FunInfo,std::less<>> _map;
	class FunInfoOpt {
		FunInfo const* _ptr;
	public:
		FunInfoOpt(FunInfo const* ptr) : _ptr(ptr) {}
		operator bool () { return _ptr; }
		FunInfo const& operator*() const { return *_ptr; }
		FunInfo const* operator->() const { return _ptr; }
	};
public:
	struct Error : std::exception {
		Exp e;
		Error(Exp const& e) : e(e) {}
	};
	static Exp const APPLIED_VAR;
	static Exp const APPLIED_CONST;
	static Exp const MISSING_TERM;
	static Exp const UNAPPLIED_FUN;
	static Exp const UNIT;
	void insert( String const& name, FunInfo const& info ) {
		_map.insert({name,info});
	}
	FunInfoOpt find( String const& name ) const {
		auto it = _map.find(name);
		return FunInfoOpt( it == _map.end() ? nullptr : &it->second );
	}
	std::optional<Exp> reads_term( ExpReader& reader ) const;
	Exp read_term( ExpReader& reader ) const {
		auto t = reads_term(reader);
		if( !t ) {
			throw Error(MISSING_TERM);
		}
		return *t;
	}
};


#endif
