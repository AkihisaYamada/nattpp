#ifndef PROBLEM_HPP_
#define PROBLEM_HPP_

#include<deque>
#include"trs.hpp"
#include"graph.hpp"

struct Problem {
	enum class Answer {
		YES, NO, MAYBE
	};
	enum {
		TRS,
		SRS,
	} format;
	Trs main;
	Map<size_t,Set<size_t>> uses_map;// will be ready by make_dps
	Graph uses_graph;// will be ready by make_dps
	Map<size_t,Set<size_t>> dp_usables;
	std::deque<Trs> components;
	using SubIt = std::deque<Trs>::iterator;
	size_t next_rule;
	enum {
		NONE = 0,
		SN,// termination
		DP,// DP problem
		SAT,// Satisfiability modulo rewriting
	} mode;
	std::set<std::pair<size_t,std::string>> extra_var;
private:
	Problem() = delete;
public:
	Problem( std::istream& is );
	void insert_rule( Trs::Rules& rules, Trs::Rule const& rule ) &;
	void make_dps() &;
	void mark_dps() &;
	std::ostream& print( std::ostream& os ) const &;
	bool reads_sym_decl( Reader& eis ) &;
	bool reads_rule_decl( Reader& eis, Trs::Reader& tis ) &;
	static bool test();
};

inline std::ostream& operator<<( std::ostream& os, Problem const& prob ) {
	return prob.print(os);
}

#endif
