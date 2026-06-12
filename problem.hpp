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
	enum {
		SN,// termination
		DP,// DP problem
	} mode;
private:
	Problem() = delete;
public:
	Problem( std::istream& is );
	std::ostream& print( std::ostream& os ) const &;
	static bool test();
};

inline std::ostream& operator<<( std::ostream& os, Problem const& prob ) {
	return prob.print(os);
}

#endif
