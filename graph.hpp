#ifndef GRAPH_HPP
#define GRAPH_HPP

#include<vector>
#include<iostream>
#include"set.hpp"
#include"map.hpp"
#include<memory>


struct GraphInterface {
	virtual Set<size_t> const& nodes() const& = 0;
	virtual Set<size_t> const& nexts( size_t src ) const& = 0;
	struct Asyclic;
};

struct GraphInterface::Asyclic : GraphInterface {
};

struct Graph final : GraphInterface {
private:
	std::unique_ptr<GraphInterface> _ptr;
public:
	/** Turn an adjacency map into a graph. */
	Graph( Map<size_t,Set<size_t>>&& map );
	/** Wrap an adjacency map as a graph. */
	Graph( Map<size_t,Set<size_t>>const& map );
	struct Acyclic;
	Set<size_t> const& nodes() const& override { return _ptr->nodes(); }
	Set<size_t> const& nexts( size_t src ) const& override { return _ptr->nexts(src); }
	std::vector<Set<size_t>> sccs() const;
	static void test();
};

struct Graph::Acyclic final : GraphInterface::Asyclic {
	
};

std::ostream& operator<<( std::ostream& os, Graph const& g );

#endif