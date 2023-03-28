#ifndef TERM_HPP_
#define TERM_HPP_
#include<map>
#include<istream>
#include "exp.hpp"

class Term {
	struct _Var {};
	static constexpr _Var _VAR = {};
public:
	typedef std::pair<std::string,std::vector<Term>> App;
	class Rank;
	class Sig;
	class Reader;
private:
	Sum<std::string,Mem<App>> _un;
	Term( std::string const& var, _Var const& ) : _un(var) {}
public:
	static Term Var( std::string const& var ) {
		return Term(var,_VAR);
	}
	Term( std::string const& fun, std::vector<Term> const& args = {} ) : _un(Mem<App>::make(fun,args)) {}
	Term( std::string const& fun, std::vector<Term> && args ) : _un(Mem<App>::make(fun,std::move(args))) {}
	Opt<std::string const&> var() const & {
		return _un.ref<std::string>();
	}
	Opt<std::string> var() && {
		return std::move(_un).ref<std::string>();
	}
	Opt<App const&> app() const & {
		if( auto p = _un.ref<Mem<App>>() ) {
			return **p;
		} else {
			return {};
		}
	}
	Opt<App> app() && {
		if( auto const& p = std::move(_un).ref<Mem<App>>() ) {
			return **p;
		} else {
			return {};
		}
	}
};

class Term::Rank {
	unsigned char _arity;
public:
	Rank() {}
	void set_arity( unsigned char arity ) { _arity = arity; }
	unsigned char arity() const { return _arity; }
};

class Term::Sig {
	std::map<std::string,Rank,std::less<>> _map;
public:
	bool insert( std::string const& name, Rank const& info ) {
		return _map.insert({name,info}).second;
	}
	Opt<Rank const&> find( std::string const& name ) const & {
		if( auto it = _map.find(name); it != _map.end() ) {
			return it->second;
		} else {
			return {};
		}
	}
};

class Term::Reader : public Exp::Reader {
	Sig const& _sig;
	/**
	 * @brief Do not construct with rvalue Sig
	 */
	Reader(std::istream&,Sig&&) = delete;
public:
	struct Error : std::exception {
		Term msg;
		Error(Term const& msg) : msg(msg) {}
	};
	Reader( std::istream& is, Sig const& sig ) : Exp::Reader(is), _sig(sig) {}
	Opt<Term> reads_term();
	Term read_term() {
		auto t = reads_term();
		if( !t ) {
			throw Error(Term("#missing-term"));
		}
		return *t;
	}
};

std::ostream& operator<<( std::ostream& os, Term const& term );

#endif
