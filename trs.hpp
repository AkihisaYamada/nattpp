#ifndef TRS_HPP_
#define TRS_HPP_
#include"util.hpp"
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
	struct Rule {
		Term first, second;
		int weight;
	public:
		Rule( Term const& l, Term const& r, int w = 1 ) : first(l), second(r), weight(w) {}
		std::ostream& print_content( std::ostream& ) const&;
		Printable print_content() const& {
			return Printable([this]( auto& os )->auto&{ return print_content(os); });
		}
	};
	struct Rules : Map<size_t,Rule> {
	};
};

inline std::ostream& operator<<( std::ostream& os, Trs::Rule const& rule ) {
	return os << "(rule " << rule.print_content() << ')';
}
inline std::ostream& operator<<( std::ostream& os, Trs::Rules const& rules ) {
	for( auto const& [n,rule] : rules ) {
		os << rule << std::endl;
	}
	return os;
}
inline std::ostream& operator<<( std::ostream& os, Trs::Rank const& rank ) {
	os << (int)rank.arity;
	if( rank.defined ) os << " defined";
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
	Opt<Term> reads( std::function<void(std::string const&)> const& var = [](auto){} );
	Term read( std::function<void(std::string const&)> const& var = [](auto){} ) {
		auto t = reads(var);
		if( !t ) {
			throw Error("#missing-term");
		}
		return *t;
	}
};

#endif
