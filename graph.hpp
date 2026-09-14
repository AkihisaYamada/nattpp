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
	friend Graph;
	using Scc = Sum<Set<uint32_t>,uint32_t>;
	struct SccInfo {
		std::vector<Scc> sccs;
		Map<uint32_t,int> scc_inds;
		std::vector<Set<uint32_t>> scc_dag;
	};
	using NodeFun = std::function<void(uint32_t)>;
protected:
	friend class _SccMaker;
	mutable Opt<SccInfo> _scc_info_opt;
	mutable Opt<Map<uint32_t,Ref<Set<uint32_t>>>> _trancl_opt;
public:
	virtual ~GraphInterface() {}
	virtual void iter_nodes( NodeFun const& ) const& = 0;
	virtual void iter_nexts( uint32_t src, NodeFun const& ) const& = 0;
	virtual SccInfo const& scc_info() const&;
	Map<uint32_t,Ref<Set<uint32_t>>> trancl() &&;
	Map<uint32_t,Ref<Set<uint32_t>>> const& trancl() const&;
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
		std::function<Set<uint32_t>const&(uint32_t)>&&
	);
	/** Turn an adjacency map into a graph. */
	Graph( Map<uint32_t,Set<uint32_t>>&& map );
	/** Turn an adjacency ref map into a graph. */
	Graph( Map<uint32_t,Ref<Set<uint32_t>>>&& map );
	/** Wrap an adjacency map as a graph. */
	Graph( Map<uint32_t,Set<uint32_t>>const& map );
	/** Wrap an adjacency ref map into a graph. */
	Graph( Map<uint32_t,Ref<Set<uint32_t>>>const& map );
	struct Acyclic;
	void iter_nodes( NodeFun const& f ) const& override {
		return _ptr->iter_nodes(f);
	}
	void iter_nexts( uint32_t src, NodeFun const& f ) const& override {
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
	Acyclic( OrdMap<uint32_t,Set<uint32_t>>&& map );
	/** Wrap an acyclic adjacency map as a graph. */
	Acyclic( OrdMap<uint32_t,Set<uint32_t>>const& map );
	/** Turn an acyclic adjacency vector into a graph. */
	Acyclic( std::vector<Set<uint32_t>>&& map );
	/** Wrap an acyclic adjacency vector as a graph. */
	Acyclic( std::vector<Set<uint32_t>>const& map );
	void iter_nodes( NodeFun const& f ) const& override {
		return _ptr->iter_nodes(f);
	}
	void iter_nexts( uint32_t src, NodeFun const& f ) const& override {
		return _ptr->iter_nexts(src,f);
	}
};

inline std::ostream& operator<<( std::ostream& os, GraphInterface const& g ) {
	return os << "(" << g.print_nodes(" (") << ')';
}

#endif