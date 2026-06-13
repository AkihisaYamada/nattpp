#ifndef PROBLEM_HPP_
#define PROBLEM_HPP_

#include"trs.hpp"

class Problem {
public:
	struct Error : ::Error {
		using ::Error::Error;
	};
	enum class Answer {
		YES, NO, MAYBE
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
	std::set<std::pair<size_t,std::string>> extra_var;
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
