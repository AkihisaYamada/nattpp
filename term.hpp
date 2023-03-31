#ifndef TERM_HPP_
#define TERM_HPP_
#include<map>
#include<istream>
#include "exp.hpp"

class TRS {
	/** @brief No object in this class */
	TRS() = delete;
public:
	class Exp : public ::Exp {
		friend TRS;
		using ::Exp::Exp;// constructors are private
	};
	class Rank;
	class Sig;
	class Reader;
	struct Rule : std::pair<Exp,Exp> { using std::pair<Exp,Exp>::pair; };
	struct Rules : std::vector<Rule> { using std::vector<Rule>::vector; };
};

std::ostream& operator<<( std::ostream& os, TRS::Rule const& rule );
std::ostream& operator<<( std::ostream& os, TRS::Rules const& sys );

class TRS::Rank {
	unsigned char _arity;
public:
	Rank() {}
	void set_arity( unsigned char arity ) { _arity = arity; }
	unsigned char arity() const { return _arity; }
};

class TRS::Sig {
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

class TRS::Reader : public Exp::Reader {
	Sig const& _sig;
	/**
	 * @brief Do not construct with rvalue Sig
	 */
	Reader(std::istream&,Sig&&) = delete;
public:
	struct Error : Exp::Error {
		using Exp::Error::Error;
	};
	Reader( std::istream& is, Sig const& sig ) : Exp::Reader(is), _sig(sig) {}
	Opt<Exp> reads_term();
	Exp read_term() {
		auto t = reads_term();
		if( !t ) {
			throw Error("#missing-term");
		}
		return *t;
	}
};

#endif
