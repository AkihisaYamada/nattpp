#ifndef TERM_HPP_
#define TERM_HPP_
#include<map>
#include<istream>
#include "exp.hpp"

struct Var : std::string {};
class Term;
typedef std::pair<std::string,std::vector<Term>> App;

class Term {
	std::variant<Var,App> _un;
public:
	class Rank;
	class Sig;
	class Reader;
	Term( Var const& var ) : _un(var) {}
	Term( std::string const& fun, std::vector<Term> const& args = {} ) : _un(App(fun,args)) {}
	Term( std::string const& fun, std::vector<Term> && args ) : _un(App(fun,std::move(args))) {}
	TempOpt<Var const> var() const & {
		if( auto p = std::get_if<Var>(&_un) ) {
			return *p;
		} else {
			return std::nullopt;
		}
	}
	std::optional<Var> var() && {
		if( auto p = std::get_if<Var>(&_un) ) {
			return *p;
		} else {
			return std::nullopt;
		}
	}
	TempOpt<App const> app() const & {
		if( auto p = std::get_if<App>(&_un) ) {
			return *p;
		} else {
			return std::nullopt;
		}
	}
	std::optional<App> app() && {
		if( auto p = std::get_if<App>(&_un) ) {
			return *p;
		} else {
			return std::nullopt;
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
	TempOpt<Rank const> find( std::string const& name ) const & {
		if( auto it = _map.find(name); it != _map.end() ) {
			return it->second;
		} else {
			return std::nullopt;
		}
	}
	Reader reader( std::istream& is ) &;
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
	std::optional<Term> reads_term();
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
