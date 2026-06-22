#ifndef GRAPH_HPP
#define GRAPH_HPP

#include<vector>
#include<iostream>
#include"set.hpp"
#include"map.hpp"
#include "util.hpp"
#include"ref.hpp"
#include"sum.hpp"

struct Graph;

struct GraphInterface {
	using Scc = Sum<Set<size_t>,size_t>;
	struct SccInfo {
		std::vector<Scc> sccs;
		Map<size_t,int> scc_inds;
		std::vector<Set<size_t>> scc_dag;
	};
	using NodeFun = std::function<void(size_t)>;
private:
	friend class _SccMaker;
	mutable Opt<SccInfo> _scc_info_opt;
	mutable OptRef<GraphInterface> _trancl_opt;
public:
	virtual ~GraphInterface() {}
	virtual void iter_nodes( NodeFun const& ) const& = 0;
	virtual void iter_nexts( size_t src, NodeFun const& ) const& = 0;
	virtual SccInfo const& scc_info() const&;
	Graph trancl() && = delete;
	Graph trancl() const&;
	std::vector<Scc> const& sccs() const& {
		return scc_info().sccs;
	}
	struct Acyclic;
	Printable print_nodes( std::string_view const& prefix = "\n  (" ) const&;
};

struct Graph final : GraphInterface {
private:
	Ref<GraphInterface> _ptr;
public:
	Graph( Ref<GraphInterface>&& org ) : _ptr(std::move(org)) {}
	/** turn node iterator function and adjacency function into a graph */
	Graph(
		std::function<void(NodeFun const&)>&&,
		std::function<Set<size_t>const&(size_t)>&&
	);
	/** Turn an adjacency map into a graph. */
	Graph( Map<size_t,Set<size_t>>&& map );
	/** Wrap an adjacency map as a graph. */
	Graph( Map<size_t,Set<size_t>>const& map );
	struct Acyclic;
	void iter_nodes( NodeFun const& f ) const& override {
		return _ptr->iter_nodes(f);
	}
	void iter_nexts( size_t src, NodeFun const& f ) const& override {
		return _ptr->iter_nexts(src,f);
	}
	static void test();
};

/** Acyclic graph interface. Nodes are supposed to be reverse-topologically ordered. */
struct GraphInterface::Acyclic : GraphInterface {
	SccInfo const& scc_info() const& override {
		assert(false/*not supported*/);
	}
	Graph::Acyclic acyc_trancl() const;
};

struct Graph::Acyclic final : GraphInterface::Acyclic {
private:
	std::unique_ptr<GraphInterface::Acyclic> _ptr;
public:
	/** Turn an acyclic adjacency map into a graph. */
	Acyclic( OrdMap<size_t,Set<size_t>>&& map );
	/** Wrap an acyclic adjacency map as a graph. */
	Acyclic( OrdMap<size_t,Set<size_t>>const& map );
	/** Turn an acyclic adjacency vector into a graph. */
	Acyclic( std::vector<Set<size_t>>&& map );
	/** Wrap an acyclic adjacency vector as a graph. */
	Acyclic( std::vector<Set<size_t>>const& map );
	void iter_nodes( NodeFun const& f ) const& override {
		return _ptr->iter_nodes(f);
	}
	void iter_nexts( size_t src, NodeFun const& f ) const& override {
		return _ptr->iter_nexts(src,f);
	}
};

inline std::ostream& operator<<( std::ostream& os, GraphInterface const& g ) {
	return os << "(" << g.print_nodes(" (") << ')';
}

#endif