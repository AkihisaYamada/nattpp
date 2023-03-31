#ifndef PROBLEM_HPP_
#define PROBLEM_HPP_

#include"trs.hpp"

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
	Problem() = delete;
public:
	Problem( std::istream& is );
	static bool test();
};
#endif
