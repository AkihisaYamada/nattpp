#ifndef TRS_HPP_
#define TRS_HPP_
#include"util.hpp"
#include"map.hpp"
#include"set.hpp"
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
		Set<size_t> defined_by;
		Set<size_t> depends;// dependency pairs 
	};
	using SigFun = std::function<Opt<Rank const&>(std::string const&)>;
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
	using Rules = Map<size_t,Rule>;
};

inline std::ostream& operator<<( std::ostream& os, Trs::Rule const& rule ) {
	return os << "(rule " << rule.print_content() << ')';
}
inline std::ostream& operator<<( std::ostream& os, Trs::Rules const& rules ) {
	for( auto const& [n,rule] : rules ) {
		os << "\n  (rule-n " << n << ' ' << rule.print_content() << ')' << std::flush;
	}
	return os;
}
inline std::ostream& operator<<( std::ostream& os, Trs::Rank const& rank ) {
	return os << (int)rank.arity;
}

class Trs::Reader {
	SigFun const& _sig;
	::Reader& _reader;
	/**
	 * @brief Do not construct with rvalue SigFun
	 */
	Reader(::Reader&,SigFun&&) = delete;
public:
	struct Error : ::Error {
		using ::Error::Error;
	};
	Reader( ::Reader& reader, SigFun const& sig ) : _reader(reader), _sig(sig) {}
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
