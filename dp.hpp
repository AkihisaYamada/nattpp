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
	struct Rules : Map<size_t,Rule> {};
};

std::ostream& operator<<( std::ostream& os, Dp::Rule const& rule );

std::ostream& operator<<( std::ostream& os, Dp::Rules const& rules );

#endif
