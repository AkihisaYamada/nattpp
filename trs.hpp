#ifndef TRS_HPP_
#define TRS_HPP_
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
	bool insert( std::string_view const& name, Rank const& info ) {
		return _map.insert({(std::string)name,info}).second;
	}
	Opt<Rank const&> find( std::string_view const& name ) const & {
		if( auto it = _map.find(name); it != _map.end() ) {
			return it->second;
		} else {
			return {};
		}
	}
	Exp term( std::string_view const& fun, std::initializer_list<Exp> args ) {
		if( auto rank = find(fun) ) {
			if( rank->arity() == args.size() ) {
				return Exp(fun,args.begin(),args.end());
			}
		}
	}
};

class TRS::Reader {
	Sig const& _sig;
	::Exp::Reader& _reader;
	/**
	 * @brief Do not construct with rvalue Sig
	 */
	Reader(std::istream&,Sig&&) = delete;
public:
	struct Error : Exp::Error {
		using Exp::Error::Error;
	};
	Reader( ::Exp::Reader& reader, Sig const& sig ) : _reader(reader), _sig(sig) {}
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
