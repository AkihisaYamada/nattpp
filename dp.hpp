#ifndef DP_HPP
#define DP_HPP

#include"problem.hpp"

class Dp {
	Dp() = delete;
public:
	struct Rule : Trs::Rule {
		std::set<Pos> deps;
		Rule( Trs::Rule const& rule, Trs::Sig const& sig );
	};
	struct Rules : std::vector<Rule> {
		using std::vector<Rule>::vector;
	};
};

std::ostream& operator<<( std::ostream& os, Dp::Rule const& rule );

std::ostream& operator<<( std::ostream& os, Dp::Rules const& rules );

#endif