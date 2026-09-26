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
	Map<uint32_t,Set<uint32_t>> uses_map;// will be ready by make_dps
	Map<uint32_t,Ref<Set<uint32_t>>> usable_graph;// will be ready by make_dps
	std::deque<Trs> components;
	using SubIt = std::deque<Trs>::iterator;
	uint32_t next_rule;
	enum {
		NONE = 0,
		SN,// termination
		DP,// DP problem
		SAT,// Satisfiability modulo rewriting
	} mode;
	std::set<std::pair<uint32_t,std::string>> extra_var;
private:
	Problem() = delete;
public:
	Problem( std::istream& is );
	void insert_rule( Trs::Rules& rules, Trs::Rule const& rule, uint32_t rule_ind ) &;
	void insert_rule( Trs::Rules& rules, Trs::Rule const& rule ) &;
	void make_dps() &;
	void init_uses() &;
	void mark_dps() &;
	std::ostream& print( std::ostream& os ) const &;
	bool reads_sym_decl( Reader& eis ) &;
	void read_rule_decl( Reader& eis, Trs::Reader& tis ) &;
	static bool test();
};

inline std::ostream& operator<<( std::ostream& os, Problem const& prob ) {
	return prob.print(os);
}

#endif
