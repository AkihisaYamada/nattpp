#ifndef TRS_HPP_
#define TRS_HPP_
#include<istream>
#include"map.hpp"
#include"exp.hpp"

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
	typedef Map<std::string,Rank> Sig;
	class Reader;
	struct Rule : std::pair<Exp,Exp> {
		int weight;
		Rule( Exp const& l, Exp const& r, int weight ) : std::pair<Exp,Exp>(l,r), weight(weight) {}
	};
	struct Rules;
};

std::ostream& operator<<( std::ostream& os, Trs::Rule const& rule );

struct Trs::Rules : std::vector<Rule> {
	using std::vector<Rule>::vector;
	std::ostream& pretty( std::ostream& os, int index, std::string const& prefix = "" ) const {
		for( auto const& rule : *this ) {
			os << '\t' << index << ": " << rule << std::endl;
			index++;
		}
		return os;
	}
	std::ostream& pretty( std::ostream& os, std::string const& prefix = "" ) const {
		for( auto const& rule : *this ) {
			os << prefix << rule << std::endl;
		}
		return os;
	}
};

inline std::ostream& operator<<( std::ostream& os, Trs::Rules const& sys ) {
	return sys.pretty(os);
}

struct Trs::Rank {
	unsigned char arity;
};

class Trs::Reader {
	Sig const& _sig;
	::Reader& _reader;
	/**
	 * @brief Do not construct with rvalue Sig
	 */
	Reader(::Reader&,Sig&&) = delete;
public:
	struct Error : ::Error {
		using ::Error::Error;
	};
	Reader( ::Reader& reader, Sig const& sig ) : _reader(reader), _sig(sig) {}
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
