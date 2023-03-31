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
	TRS::Sig sig;
	std::vector<TRS::Rules> systems;
private:
	void _read_format( Exp::Reader& reader );
	void _process_trs_format( Exp::Reader& reader );
	void _process_srs_format( Exp::Reader& reader ) {
		format = SRS;
	}
	void _process_fun(Exp::Reader& reader);
	void _process_rule(TRS::Reader& reader);
	void _parse( std::istream& is );
	Problem() = delete;
public:
	Problem( std::istream& is ) {
		_parse(is);
	}
};
#endif
