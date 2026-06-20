#ifndef PROBLEM_HPP_
#define PROBLEM_HPP_

#include<list>
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
	std::list<System> systems;
	using SysIt = std::list<System>::iterator;
	size_t next_rule;
	enum {
		SN,// termination
		DP,// DP problem
	} mode;
	std::set<std::pair<size_t,std::string>> extra_var;
private:
	Problem() = delete;
public:
	Problem( std::istream& is );
	void insert_rule( Trs::Rules& rules, Trs::Rule const& rule ) &;
	void make_dps() &;
	void mark_dps( SysIt const& it ) &;
	std::ostream& print( std::ostream& os ) const &;
	static bool test();
};

inline std::ostream& operator<<( std::ostream& os, Problem const& prob ) {
	return prob.print(os);
}

#endif
