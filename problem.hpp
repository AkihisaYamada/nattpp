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
	Trs main;
	Map<size_t,Set<size_t>> static_usable;// will be ready by make_dps
	std::list<Trs> subtrss;
	using SubIt = std::list<Trs>::iterator;
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
	void mark_dps( SubIt const& it ) &;
	std::ostream& print( std::ostream& os ) const &;
	static bool test();
};

inline std::ostream& operator<<( std::ostream& os, Problem const& prob ) {
	return prob.print(os);
}

#endif
