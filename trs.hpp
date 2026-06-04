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
		std::ostream& print( std::ostream& os ) const;
		std::ostream& print( std::ostream& os, int index ) const;
	};
	struct Rules;
};

struct Trs::Rules : std::vector<Rule> {
	using std::vector<Rule>::vector;
	std::ostream& print( std::ostream& os, int index, std::string const& prefix = "" ) const {
		for( auto const& rule : *this ) {
			rule.print( os << prefix, index ) << std::endl;
			index++;
		}
		return os;
	}
	std::ostream& print( std::ostream& os, std::string const& prefix = "" ) const {
		for( auto const& rule : *this ) {
			rule.print( os << prefix ) << std::endl;
		}
		return os;
	}
};

inline std::ostream& operator<<( std::ostream& os, Trs::Rule const& rule ) {
	return rule.print(os);
}
inline std::ostream& operator<<( std::ostream& os, Trs::Rules const& sys ) {
	return sys.print(os);
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
