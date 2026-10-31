#ifndef GRAPH_HPP
#define GRAPH_HPP

#include<vector>
#include<stack>
#include<iostream>
#include"set.hpp"
#include"map.hpp"
#include "util.hpp"
#include"ref.hpp"
#include"sum.hpp"

template<typename N>
struct ConstGraph;

template<typename N>
struct Graph;

template<typename N>
struct ConstGraphInterface {
	friend ConstGraph<N>;
	using Scc = Sum<Set<N>,N>;// nontrivial or trivial
	struct SccInfo {
		std::vector<Scc> sccs;
		Map<N,unsigned> scc_inds;// node to SCC index
		std::vector<Set<unsigned>> scc_dag;
	};
protected:
	class _SccMaker;
	mutable Opt<SccInfo> _scc_info_opt;
	mutable Opt<Map<N,Ref<Set<N>>>> _trancl_opt;
public:
	virtual ~ConstGraphInterface() {}
	virtual void iter_nodes( std::function<void(N)> const& ) const& = 0;
	virtual void iter_nexts( N src, std::function<void(N)> const& ) const& = 0;
	virtual SccInfo const& scc_info() const&;
	Map<N,Ref<Set<N>>> const& trancl() const&;
	Map<N,Ref<Set<N>>> trancl() && {
		return trancl();
	}
	std::vector<Scc> const& sccs() const& {
		return scc_info().sccs;
	}
	uint32_t scc_ind( N node ) const& {
		return *ASSERTED(scc_info().scc_inds.find(node));
	}
	struct Acyclic;
	Printable print_nodes( std::string_view const& pref, std::string_view const& _sep ) const& {
		return Printable([&]( std::ostream& os )->std::ostream&{
			std::string_view sep = pref;
			iter_nodes([&]( auto const& src ){
				os << sep << "(node " << src;
				sep = _sep;
				iter_nexts(src,[&]( auto const& tgt ){
					os << ' ' << tgt;
				});
				os << ')' << std::flush;
			});
			return os;
		});
	}
	Printable print_nodes( std::string_view const& sep ) const& {
		return print_nodes(sep,sep);
	}
};

template<typename N>
inline std::ostream& operator<<( std::ostream& os, ConstGraphInterface<N> const& g ) {
	return os << "(" << g.print_nodes(""," ") << ')';
}

template<typename N>
struct GraphInterface : ConstGraphInterface<N> {
	virtual bool erase_node( N node ) & = 0;
	virtual bool erase_edges( N src, std::function<bool(N)> const& test ) & = 0;
};

template<typename N>
struct ConstGraph final : ConstGraphInterface<N> {
private:
	Ref<ConstGraphInterface<N>> _ptr;
public:
	ConstGraph( Ref<ConstGraphInterface<N>>&& org ) : _ptr(std::move(org)) {}
	/** turn node iterator function and adjacency function into a graph */
	ConstGraph(
		std::function<void(std::function<void(N)> const&)>&&,
		std::function<Set<N>const&(N)>&&
	);
	/** Turn an adjacency map into a graph. */
	ConstGraph( Map<N,Set<N>>&& map );
	/** Wrap an adjacency map as a graph. */
	ConstGraph( Map<N,Set<N>>const& map );
	/** Turn an adjacency ref map into a graph. */
	ConstGraph( Map<N,Ref<Set<N>>>&& map );
	/** Wrap an adjacency ref map into a graph. */
	ConstGraph( Map<N,Ref<Set<N>>>const& map );
	struct Acyclic;
	void iter_nodes( std::function<void(N)> const& f ) const& override {
		return _ptr->iter_nodes(f);
	}
	void iter_nexts( N src, std::function<void(N)> const& f ) const& override {
		return _ptr->iter_nexts(src,f);
	}
};

template<typename N>
struct Graph : GraphInterface<N> {
private:
	Ref<GraphInterface<N>> _ptr;
public:
	Graph( Ref<GraphInterface<N>>&& org ) : _ptr(std::move(org)) {}
	/** Turn an adjacency map into a graph. */
	Graph( Map<N,Set<N>>&& map );
	/** Wraps an adjacency map as a graph. */
	Graph( Map<N,Set<N>>& map );
	struct Acyclic;
	void iter_nodes( std::function<void(N)> const& f ) const& override {
		return _ptr->iter_nodes(f);
	}
	void iter_nexts( N src, std::function<void(N)> const& f ) const& override {
		return _ptr->iter_nexts(src,f);
	}
	bool erase_node( N node ) & override {
		return _ptr->erase_node(node);
	}
	bool erase_edges( N src, std::function<bool(N)> const& test ) & override {
		return _ptr->erase_edges(src,test);
	}
};

/** Acyclic graph interface. Nodes are supposed to be reverse-topologically ordered. */
template<typename N>
struct ConstGraphInterface<N>::Acyclic : ConstGraphInterface<N> {
	SccInfo const& scc_info() const& override {
		assert(false/*not supported*/);
	}
	ConstGraph<N>::Acyclic acyc_trancl() const;
};

template<typename N>
struct ConstGraph<N>::Acyclic final : ConstGraphInterface<N>::Acyclic {
private:
	std::unique_ptr<typename ConstGraphInterface<N>::Acyclic> _ptr;
public:
	/** Turn an acyclic adjacency map into a graph. */
	Acyclic( OrdMap<N,Set<N>>&& map );
	/** Wrap an acyclic adjacency map as a graph. */
	Acyclic( OrdMap<N,Set<N>>const& map );
	/** Turn an acyclic adjacency vector into a graph. */
	Acyclic( std::vector<Set<N>>&& map );
	/** Wrap an acyclic adjacency vector as a graph. */
	Acyclic( std::vector<Set<N>>const& map );
	void iter_nodes( std::function<void(N)> const& f ) const& override {
		return _ptr->iter_nodes(f);
	}
	void iter_nexts( N src, std::function<void(N)> const& f ) const& override {
		return _ptr->iter_nexts(src,f);
	}
};

template<typename N>
Map<N,Ref<Set<N>>> const& ConstGraphInterface<N>::trancl() const& {
	if( !_trancl_opt ) {
		_trancl_opt = {{}};
		auto const& [sccs,scc_inds,scc_dag] = scc_info();
		auto scc_trancl = typename ConstGraph<unsigned>::Acyclic(scc_dag).acyc_trancl();
		std::vector<Ref<Set<N>>> scc_reachables;
		scc_trancl.iter_nodes([&]( unsigned scc )->void{
			auto& reachables = scc_reachables.emplace_back(Ref<Set<N>>::make());
			scc_trancl.iter_nexts(scc,[&]( unsigned next_scc ){
				// nodes reachable from the next SCC are reachable
				for( auto const& node : *scc_reachables[next_scc] ) {
					reachables->emplace(node);
				}
				// if next is a trivial SCC, then mark the node reachable, since it is not so from the SCC 
				if( auto const& triv = sccs[next_scc].template ref<N>() ) {
					reachables->emplace(*triv);
				}
			});
			if( auto const& triv = sccs[scc].template ref<N>() ) {// trivial SCC
					_trancl_opt->emplace(*triv,reachables);// the node reaches where the SCC reaches
			} else if( auto const& nodes = sccs[scc].template ref<Set<N>>() ) {
				for( auto const& node : *nodes ) {
					_trancl_opt->emplace(node,reachables);// the node reaches where the SCC reaches
					reachables->emplace(node);// the node is reachable
				}
			} else {
				assert(false);
			}
		});
	}
	return *_trancl_opt;
}

template<typename N>
struct _FunGraph final : ConstGraphInterface<N> {
private:
	std::function<void(std::function<void(N)> const&)> _node_iter;
	std::function<Set<N>const&(N)> _nexts;
public:
	_FunGraph(
		std::function<void(std::function<void(N)> const&)>&& node_iter,
		std::function<Set<N>const&(N)>&& nexts
	) : _node_iter(std::move(node_iter)), _nexts(std::move(nexts)) {}
	void iter_nodes( std::function<void(N)> const& f ) const& override {
		_node_iter(f);
	}
	void iter_nexts( N src, std::function<void(N)> const& f ) const& override {
		for( auto const& next : _nexts(src) ) f(next);
	}
};
template<typename N>
ConstGraph<N>::ConstGraph( std::function<void(std::function<void(N)> const&)>&& node_iter, std::function<Set<N>const&(N)>&& nexts ) :
	_ptr(Ref<_FunGraph<N>>::make(std::move(node_iter),std::move(nexts))) {}

template<typename N>
struct _ConstMapGraph final : ConstGraphInterface<N> {
	using Body = Map<N,Set<N>>;
private:
	Body const& _body;
public:
	_ConstMapGraph( Body const& org ) : _body(org) {}
	void iter_nodes( std::function<void(N)> const& f ) const& override {
		for( auto const& [node,nexts] : _body ) f(node);
	}
	void iter_nexts( N src, std::function<void(N)> const& f ) const& override {
		if( auto const& nexts = _body.find(src) ) {
			for( auto const& next : *nexts ) f(next);
		}
	}
};
template<typename N>
ConstGraph<N>::ConstGraph( Map<N,Set<N>>const& map ) :
	_ptr( Ref<_ConstMapGraph<N>>::make(map) ) {}

template<typename N>
struct _ConstRefMapGraph final : ConstGraphInterface<N> {
	using Body = Map<N,Ref<Set<N>>>;
private:
	Body const& _body;
public:
	_ConstRefMapGraph( Map<N,Ref<Set<N>>>const& org ) : _body(org) {}
	void iter_nodes( std::function<void(N)> const& f ) const& override {
		for( auto const& [node,nexts] : _body ) f(node);
	}
	void iter_nexts( N src, std::function<void(N)> const& f ) const& override {
		if( auto const& nexts = _body.find(src) ) {
			for( auto const& next : **nexts ) f(next);
		}
	}
};
template<typename N>
ConstGraph<N>::ConstGraph( Map<N,Ref<Set<N>>>const& map ) :
	_ptr( Ref<_ConstRefMapGraph<N>>::make(map) ) {}

template<typename N>
struct _RefMapConstGraph final : ConstGraphInterface<N> {
	using Body = Map<N,Ref<Set<N>>>;
private:
	Body _body;
public:
	_RefMapConstGraph( Map<N,Ref<Set<N>>>&& org ) : _body(std::move(org)) {}
	void iter_nodes( std::function<void(N)> const& f ) const& override {
		for( auto const& [node,nexts] : _body ) f(node);
	}
	void iter_nexts( N src, std::function<void(N)> const& f ) const& override {
		if( auto const& nexts = _body.find(src) ) {
			for( auto const& next : **nexts ) f(next);
		}
	}
};
template<typename N>
ConstGraph<N>::ConstGraph( Map<N,Ref<Set<N>>>&& map ) :
	_ptr( Ref<_RefMapConstGraph<N>>::make(std::move(map)) ) {}

template<typename N>
struct _AcyclicMapConstRefGraph final : ConstGraphInterface<N>::Acyclic {
	using Body = OrdMap<N,Set<N>>;
private:
	Body const& _body;
public:
	_AcyclicMapConstRefGraph( Body const& org ) : _body(org) {}
	void iter_nodes( std::function<void(N)> const& f ) const& override {
		for( auto const& [node,nexts] : _body ) f(node);
	}
	void iter_nexts( N src, std::function<void(N)> const& f ) const& override {
		if( auto const& nexts = _body.find(src) ) {
			for( auto const& next : *nexts ) f(next);
		}
	}
};
template<typename N>
ConstGraph<N>::Acyclic::Acyclic( OrdMap<N,Set<N>>const& map ) :
	_ptr( std::make_unique<_AcyclicMapConstRefGraph<N>>(map) ) {}

template<typename N>
struct _AcyclicMapGraph final : ConstGraphInterface<N>::Acyclic {
	using Body = OrdMap<N,Set<N>>;
private:
	Body _body;
public:
	_AcyclicMapGraph( Body&& map ) : _body(std::move(map)) {}
	void iter_nodes( std::function<void(N)> const& f ) const& override {
		for( auto const& [node,nexts] : _body ) f(node);
	}
	void iter_nexts( N src, std::function<void(N)> const& f ) const& override {
		if( auto const& nexts = _body.find(src) ) {
			for( auto const& next : *nexts ) f(next);
		}
	}
};
template<typename N>
ConstGraph<N>::Acyclic::Acyclic( OrdMap<N,Set<N>>&& map ) :
	_ptr( std::make_unique<_AcyclicMapGraph<N>>(std::move(map)) ) {}

template<typename N>
struct _AcyclicVecConstRefGraph final : ConstGraphInterface<N>::Acyclic {
	using Body = std::vector<Set<N>>;
private:
	Body const& _body;
public:
	_AcyclicVecConstRefGraph( Body const& org ) : _body(org) {}
	void iter_nodes( std::function<void(N)> const& f ) const& override {
		for( auto node = 0; node < _body.size(); node++ ) f(node);
	}
	void iter_nexts( N src, std::function<void(N)> const& f ) const& override {
		if( src < _body.size() ) {
			for( auto const& next : _body[src] ) f(next);
		}
	}
};
template<typename N>
ConstGraph<N>::Acyclic::Acyclic( std::vector<Set<N>>const& map ) :
	_ptr( std::make_unique<_AcyclicVecConstRefGraph<N>>(map) ) {}

template<typename N>
struct _AcyclicVecGraph final : ConstGraphInterface<N>::Acyclic {
	using Body = std::vector<Set<N>>;
private:
	Body _body;
public:
	_AcyclicVecGraph( Body&& org ) : _body(std::move(org)) {}
	void iter_nodes( std::function<void(N)> const& f ) const& override {
		for( auto node = 0; node < _body.size(); node++ ) f(node);
	}
	void iter_nexts( N src, std::function<void(N)> const& f ) const& override {
		if( src < _body.size() ) {
			for( auto const& next : _body[src] ) f(next);
		}
	}
};

template<typename N>
ConstGraph<N>::Acyclic::Acyclic( std::vector<Set<N>>&& map ) :
	_ptr( std::make_unique<_AcyclicVecGraph<N>>(std::move(map)) ) {}

template<typename N>
struct _MapGraph final : GraphInterface<N> {
	using Body = Map<N,Set<N>>;
private:
	Body _body;
public:
	_MapGraph( Map<N,Set<N>>&& org ) : _body(std::move(org)) {}
	void iter_nodes( std::function<void(N)> const& f ) const& override {
		for( auto const& [node,nexts] : _body ) f(node);
	}
	void iter_nexts( N src, std::function<void(N)> const& f ) const& override {
		if( auto const& nexts = _body.find(src) ) {
			for( auto const& next : *nexts ) f(next);
		}
	}
	bool erase_node( N node ) & override {
		return _body.erase(node);
	}
	bool erase_edges( N src, std::function<bool(N)> const& test ) & override {
		return _body.find(src) && [&]( Set<N>& nexts )->bool{ return nexts.erase_if(test); };
	}
};
template<typename N>
Graph<N>::Graph( Map<N,Set<N>>&& map ) :
	_ptr( Ref<_MapGraph<N>>::make(std::move(map)) ) {
}
template<typename N>
ConstGraph<N>::ConstGraph( Map<N,Set<N>>&& map ) :
	_ptr( Ref<_MapGraph<N>>::make(std::move(map)) ) {
}

template<typename N>
struct _MapRefGraph final : GraphInterface<N> {
	using Body = Map<N,Set<N>>;
private:
	Body& _body;
public:
	_MapRefGraph( Map<N,Set<N>>& org ) : _body(org) {}
	void iter_nodes( std::function<void(N)> const& f ) const& override {
		for( auto const& [node,nexts] : _body ) f(node);
	}
	void iter_nexts( N src, std::function<void(N)> const& f ) const& override {
		if( auto const& nexts = _body.find(src) ) {
			for( auto const& next : *nexts ) f(next);
		}
	}
	bool erase_node( N node ) & override {
		return _body.erase(node);
	}
	bool erase_edges( N src, std::function<bool(N)> const& test ) & override {
		return _body.find(src) && [&]( Set<N>& nexts )->bool{ return nexts.erase_if(test); };
	}
};
template<typename N>
Graph<N>::Graph( Map<N,Set<N>>& map ) :
	_ptr( Ref<_MapRefGraph<N>>::make(map) ) {
}

/* An SCC decomposition similar to Pearce's https://doi.org/10.1016/j.ipl.2015.08.010 */
template<typename N>
struct ConstGraphInterface<N>::_SccMaker {
	ConstGraphInterface<N> const& g;
	static constexpr unsigned OPEN = 1u << 31;
	static bool is_closed( unsigned s ) { return !( s & OPEN ); }
	/** Node status table
	 * OPEN bit = 0: represents the node's SCC, which is already closed
	 * OPEN bit = 1: represents the farthest depth the node can reach back
	 */
	Map<N,unsigned>& table;
	/** Stack of nodes with backlinks */
	std::stack<N> node_stack;
	/** Stack containing adjacent SCCs */
	std::stack<unsigned> scc_stack;
	std::vector<typename ConstGraph<N>::Scc>& sccs;
	std::vector<Set<unsigned>>& scc_dag;
	unsigned depth = ~0u;// 1...111

	int visit( N u ) {
		auto const [state,fl] = table.emplace(u,depth);
		if( !fl ) {// already visited
			return state;
		}
		bool loop = false, back = false;
		depth--;
		assert( depth & OPEN );
		// remember how many nodes were stacked
		unsigned node_stack_size = node_stack.size();
		unsigned scc_stack_size = scc_stack.size();
		g.iter_nexts( u, [&]( auto v ){
			auto vstate = visit(v);
			if( is_closed(vstate) ) {// v's SCC is known
				scc_stack.emplace(vstate);// remember the SCC
			} else if( state < vstate ) {// v goes further
				state = vstate;// u can go back as far as v can
				back = true;
			} else if( state == vstate ) {
				loop = true;
			}
		} );
		if( back ) {// u constitute a bigger SCC
			node_stack.emplace(u);
			return state;
		}
		// u was the entry point of the (possibly trivial) SCC
		state = sccs.size();// remember this SCC index
		auto& scc_node = scc_dag.emplace_back();
		while( scc_stack.size() != scc_stack_size ) {// connect SCCs
			auto next = scc_stack.top();
			scc_stack.pop();
			scc_node.emplace(next);
		}
		if( loop ) {// nontrivial
			auto& scc = *sccs.emplace_back(Set<N>{}).template ref<Set<N>>();
			scc.emplace(u);
			while( node_stack.size() != node_stack_size ) {// things pushed after u belongs to the SCC
				auto v = node_stack.top();
				node_stack.pop();
				scc.emplace(v);
				*ASSERTED(table.find(v)) = state;
			}
		} else {
			sccs.emplace_back(u);// trivial SCC
		}
		return state;
	}
	_SccMaker( ConstGraphInterface const& g ) : depth(-1), g(g),
		sccs(g._scc_info_opt->sccs),
		table(g._scc_info_opt->scc_inds),
		scc_dag(g._scc_info_opt->scc_dag) {
	}
};

template<typename N>
ConstGraphInterface<N>::SccInfo const& ConstGraphInterface<N>::scc_info() const& {
	if( _scc_info_opt ) return *_scc_info_opt;
	_scc_info_opt = {{}};
	auto maker = _SccMaker(*this);
	iter_nodes([&]( auto const& node ){
		maker.visit(node);
	});
	return *_scc_info_opt;
}
template<typename N>
ConstGraph<N>::Acyclic ConstGraphInterface<N>::Acyclic::acyc_trancl() const {
	OrdMap<N,Set<N>> map;
	iter_nodes([&]( auto const& src ){
		auto [nexts,fl] = map.emplace(src,Set<N>());
		assert(fl);
		iter_nexts(src,[&]( auto const& tgt ){
			for( auto const& next : *ASSERTED(map.find(tgt)) ) {
				nexts.emplace(next);
			}
			nexts.emplace(tgt);
		});
	});
	return map;
}

template<typename N>
Printable print_sccs( std::vector<typename ConstGraph<N>::Scc> const& sccs ){
	return Printable([&]( std::ostream& os )->std::ostream&{
		os << "(sccs";
		for( auto const& scc : sccs ) {
			if( auto triv = scc.template ref<N>() ) {
				os << "\n  " << *triv << std::flush;
			} else if( auto nodes = scc.template ref<Set<N>>() ){
				os << "\n  (" << print_list(*nodes) << ')' << std::flush;
			}
		}
		return os << ')' << std::endl;
	});
}
extern void graph_test();

#endif