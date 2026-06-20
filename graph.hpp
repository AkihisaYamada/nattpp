#ifndef GRAPH_HPP
#define GRAPH_HPP

#include<vector>
#include"map.hpp"
#include"set.hpp"

struct Graph {
	Map<size_t,Set<size_t>> map;
	std::vector<Set<size_t>> sccs() const;
	static void test();
};


#endif