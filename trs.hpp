#ifndef TRS_HPP_
#define TRS_HPP_
#include"util.hpp"
#include"map.hpp"
#include"set.hpp"
#include"exp.hpp"

enum class ruleId : uint32_t {};
constexpr ruleId operator+( ruleId id, uint32_t n ) { return ruleId((uint32_t)id + n); }
constexpr ruleId& operator+=( ruleId& id, uint32_t n ) { return id = id + n; }
constexpr ruleId& operator++( ruleId& id ) { return id += 1; }
constexpr ruleId operator++( ruleId& id, int ) { auto ret = id; ++id; return ret; }
inline std::ostream& operator<<( std::ostream& os, ruleId id ) {
	return os << (uint32_t)id;
}
namespace std {
	inline string to_string( ruleId id ) {
		return to_string((uint32_t)id);
	}
}

struct Trs {
	class Term : public ::Exp {
		friend Trs;
		friend class Srs;
		using ::Exp::Exp;// constructors are private
	};
	struct Rank {
		unsigned char arity;
		Set<ruleId> defined_by;
		std::ostream& print_content( std::ostream& os, bool definers = false ) const&;
		Printable print_content( bool definers = false ) const& {
			return Printable([this,definers]( auto& os )->auto&{ return print_content(os,definers); });
		}
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
	using Rules = Map<ruleId,Rule>;
	Sig sig;
	Rules rules;
	void insert_rule( ruleId i, Rule const& rule ) &;
	void erase_rule( ruleId i ) &;
	static std::ostream& print_sig( std::ostream& os, Sig const& sig, std::string_view const& sep );
	static std::ostream& print_rules( std::ostream& os, Rules const& rules, std::string_view const& sep );
};

inline std::ostream& operator<<( std::ostream& os, Trs::Rule const& rule ) {
	return os << "(rule " << rule.print_content() << ')';
}
inline std::ostream& operator<<( std::ostream& os, Trs::Rules const& rules ) {
	for( auto const& [n,rule] : rules ) {
		os << "\n  (rule " << rule.print_content() << " :number " << n << ')' << std::flush;
	}
	return os;
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
