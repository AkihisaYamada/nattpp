#ifndef PROBLEM_HPP_
#define PROBLEM_HPP_

#include<deque>
#include"trs.hpp"

struct Answer : Exp {
private:
	Answer() = delete;
	Answer( Exp const& other ) : Exp(other) {}
public:
	static Answer const YES, NO, MAYBE;
};

struct Problem : public Trs {
	enum class Answer {
		YES, NO, MAYBE
	};
	enum {
		TRS,
		SRS,
	} format;
	Set<ruleId> rule_ids;// will be set in DP mode
	Map<ruleId,Set<ruleId>> uses_map;// will be ready by make_dps
	Map<ruleId,Ref<Set<ruleId>>> usable_graph;// will be ready by make_dps
	struct Component {
		Map<ruleId,Set<ruleId>> graph;
	};
	std::deque<Component> components;
	ruleId next_rule;
	enum {
		NONE = 0,
		SN,// termination
		DP,// DP problem
		SAT,// Satisfiability modulo rewriting
	} mode;
	std::set<std::pair<ruleId,std::string>> extra_var;
private:
	Problem() = delete;
public:
	Problem( std::istream& is );
	void insert_rule( Trs::Rule const& rule, ruleId rule_ind ) &;
	void insert_rule( Trs::Rule const& rule ) &;
	void make_dps() &;
	void decomp_sccs() &;
	void init_uses() &;
	std::ostream& print( std::ostream& os ) const &;
	bool reads_sym_decl( ::Reader& eis ) &;
	void read_rule_decl( ::Reader& eis, Trs::Reader& tis ) &;
	bool freeze( bool leftmost ) &;
	static bool test();
};

inline std::ostream& operator<<( std::ostream& os, Problem const& prob ) {
	return prob.print(os);
}

#endif
