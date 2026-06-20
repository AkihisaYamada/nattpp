#ifndef PROBLEM_HPP_
#define PROBLEM_HPP_

#include"trs.hpp"

struct Problem {
	enum class Answer {
		YES, NO, MAYBE
	};
	enum {
		TRS,
		SRS,
	} format;
	struct System {
		Trs::Sig sig;// additional signature
		Trs::Rules rules;// additional rules
	};
	std::vector<System> systems;
	size_t last_rule;
	enum {
		SN,// termination
		DP,// DP problem
	} mode;
	std::set<std::pair<size_t,std::string>> extra_var;
private:
	Problem() = delete;
public:
	Problem( std::istream& is );
	void add_rule( size_t index, Trs::Rule const& rule ) &;
	void make_dps() &;
	void mark_dps() &;
	std::ostream& print( std::ostream& os ) const &;
	static bool test();
};

inline std::ostream& operator<<( std::ostream& os, Problem const& prob ) {
	return prob.print(os);
}

#endif
