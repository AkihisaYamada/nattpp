#ifndef PARSER_HPP_
#define PARSER_HPP_

#include"term.hpp"

class Parser {
public:
	struct Error : Exp::Error {
		using Exp::Error::Error;
	};
	enum class Format {
		TRS,
		SRS,
	};

	Term::Sig sig;
	struct System : std::vector<std::pair<Term,Term>> {};
	std::vector<System> systems;
	Format format;
	/**
	 * @brief Reads (format ...) expression.
	 */
	void _read_format( Exp::Reader& reader );
	void _process_trs_format( Exp::Reader& reader );
	void _process_srs_format( Exp::Reader& reader ) {
		format = Format::SRS;
	}
	void _process_fun(Exp::Reader& reader);
	void _process_rule(Term::Reader& reader);
public:
	void parse( std::istream& is );
	void write_systems( std::ostream& os );
};
#endif
