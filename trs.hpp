#ifndef TRS_HPP_
#define TRS_HPP_
#include<map>
#include<istream>
#include "exp.hpp"

class Trs {
	/** @brief No object in this class */
	Trs() = delete;
public:
	class Exp : public ::Exp {
		friend Trs;
		friend class Srs;
		using ::Exp::Exp;// constructors are private
	};
	class Rank;
	class Sig;
	class Reader;
	struct Rule : std::pair<Exp,Exp> { using std::pair<Exp,Exp>::pair; };
	struct Rules : std::vector<Rule> { using std::vector<Rule>::vector; };
};

std::ostream& operator<<( std::ostream& os, Trs::Rule const& rule );
std::ostream& operator<<( std::ostream& os, Trs::Rules const& sys );

struct Trs::Rank {
	unsigned char arity;
};

class Trs::Sig {
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
};

class Trs::Reader {
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
	Opt<Exp> reads();
	Exp read() {
		auto t = reads();
		if( !t ) {
			throw Error("#missing-term");
		}
		return *t;
	}
};

#endif
