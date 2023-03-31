#ifndef PROBLEM_HPP_
#define PROBLEM_HPP_

#include"term.hpp"

class Problem {
public:
	struct Error : Exp::Error {
		using Exp::Error::Error;
	};
	enum {
		TRS,
		SRS,
	} format;
	Term::Sig sig;
	struct System : std::vector<std::pair<Term,Term>> {
		using std::vector<std::pair<Term,Term>>::vector;
	};
	std::vector<System> systems;
private:
	void _read_format( Exp::Reader& reader );
	void _process_trs_format( Exp::Reader& reader );
	void _process_srs_format( Exp::Reader& reader ) {
		format = SRS;
	}
	void _process_fun(Exp::Reader& reader);
	void _process_rule(Term::Reader& reader);
	void _parse( std::istream& is );
	Problem() = delete;
public:
	Problem( std::istream& is ) {
		_parse(is);
	}
	void write_systems( std::ostream& os );
};
#endif
