#ifndef GRAPH_HPP
#define GRAPH_HPP

#include<vector>
#include<iostream>
#include"set.hpp"
#include"map.hpp"
#include<memory>

struct Graph {
	Set<size_t> nodes;
private:
	using _Fun = std::function<Set<size_t>const&(size_t)>;
	_Fun _fun;
	Graph( _Fun&& fun ) : _fun(std::move(fun)) {}
public:
	/** Turn an adjacency map into a graph. */
	Graph( Map<size_t,Set<size_t>>&& map );
	/** Wrap an adjacency map as a graph. */
	Graph( Map<size_t,Set<size_t>>const& map );
	struct Acyclic;
	Set<size_t> const& nexts( size_t src ) const& { return _fun(src); }
	std::vector<Set<size_t>> sccs() const;
	static void test();
};

struct Graph::Acyclic : Graph {
	
};

std::ostream& operator<<( std::ostream& os, Graph const& g );

#endif