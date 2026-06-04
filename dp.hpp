#ifndef DP_HPP
#define DP_HPP

#include"problem.hpp"

struct DepMap : std::vector<std::set<Pos>> {
};

DepMap dep_map( Trs::Sig const& sig, Trs::Rules const& trs );

#endif