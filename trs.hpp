#ifndef TRS_HPP_
#define TRS_HPP_
#include<istream>
#include"map.hpp"
#include"exp.hpp"

class Trs {
	/** @brief No object in this class */
	Trs() = delete;
public:
	class Term : public ::Exp {
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
	struct Rule : std::pair<Term,Term> {
		int weight;
		Rule( Term const& l, Term const& r, int weight ) : std::pair<Term,Term>(l,r), weight(weight) {}
		std::ostream& print_contents( std::ostream& os ) const;
	};
	struct Rules : Map<size_t,Rule> {
	};
};

inline std::ostream& operator<<( std::ostream& os, Trs::Rule const& rule ) {
	return rule.print_contents( os << "(rule " ) << ')';
}
inline std::ostream& operator<<( std::ostream& os, Trs::Rules const& rules ) {
	for( auto const& [n,rule] : rules ) {
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
	Opt<Term> reads();
	Term read() {
		auto t = reads();
		if( !t ) {
			throw Error("#missing-term");
		}
		return *t;
	}
};

#endif
