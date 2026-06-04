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
	struct Rank {
		unsigned char arity;
		bool defined;
	};
	using Sig = Map<std::string,Rank>;
	class Reader;
	struct Rule : std::pair<Exp,Exp> {
		int weight;
		Rule( Exp const& l, Exp const& r, int weight ) : std::pair<Exp,Exp>(l,r), weight(weight) {}
		std::ostream& print_contents( std::ostream& os ) const;
	};
	struct Rules : std::vector<Rule> {
		using std::vector<Rule>::vector;
	};
};

inline std::ostream& operator<<( std::ostream& os, Trs::Rule const& rule ) {
	return rule.print_contents( os << "(rule " ) << ')';
}
inline std::ostream& operator<<( std::ostream& os, Trs::Rules const& rules ) {
	for( auto const& rule : rules ) {
		os << rule << std::endl;
	}
	return os;
}

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
