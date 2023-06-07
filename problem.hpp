#ifndef PROBLEM_HPP_
#define PROBLEM_HPP_

#include"trs.hpp"

class Problem {
public:
	struct Error : ::Error {
		using ::Error::Error;
	};
	enum {
		TRS,
		SRS,
	} format;
	Trs::Sig sig;
	std::vector<Trs::Rules> systems;
private:
	Problem() = delete;
public:
	Problem( std::istream& is );
	static bool test();
};
#endif
