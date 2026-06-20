#ifndef GRAPH_HPP
#define GRAPH_HPP

#include<vector>
#include<iostream>
#include"map.hpp"
#include"set.hpp"

struct Graph {
	Map<size_t,Set<size_t>const&> map;
	std::vector<Set<size_t>> sccs() const;
	static void test();
};

std::ostream& operator<<( std::ostream& os, Graph const& g );

#endif