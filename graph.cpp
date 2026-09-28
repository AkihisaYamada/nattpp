#include<vector>
#include<limits>
#include<stack>
#include"map.hpp"
#include"sum.hpp"
#include"graph.hpp"
#include"util.hpp"

using namespace std;

static Set<uint32_t> const EMPTY = {};

struct _FunGraph final : ConstGraphInterface {
private:
	std::function<void(NodeFun const&)> _node_iter;
	std::function<Set<uint32_t>const&(uint32_t)> _nexts;
public:
	_FunGraph(
		std::function<void(NodeFun const&)>&& node_iter,
		std::function<Set<uint32_t>const&(uint32_t)>&& nexts
	) : _node_iter(std::move(node_iter)), _nexts(std::move(nexts)) {}
	void iter_nodes( NodeFun const& f ) const& override {
		_node_iter(f);
	}
	void iter_nexts( uint32_t src, NodeFun const& f ) const& override {
		for( auto const& next : _nexts(src) ) f(next);
	}
};
ConstGraph::ConstGraph( std::function<void(NodeFun const&)>&& node_iter, std::function<Set<uint32_t>const&(uint32_t)>&& nexts ) :
	_ptr(Ref<_FunGraph>::make(std::move(node_iter),std::move(nexts))) {}

struct _ConstMapGraph final : ConstGraphInterface {
	using Body = Map<uint32_t,Set<uint32_t>>;
private:
	Body const& _body;
public:
	_ConstMapGraph( Body const& org ) : _body(org) {}
	void iter_nodes( NodeFun const& f ) const& override {
		for( auto const& [node,nexts] : _body ) f(node);
	}
	void iter_nexts( uint32_t src, NodeFun const& f ) const& override {
		if( auto const& nexts = _body.find(src) ) {
			for( auto const& next : *nexts ) f(next);
		}
	}
};
ConstGraph::ConstGraph( Map<uint32_t,Set<uint32_t>>const& map ) :
	_ptr( Ref<_ConstMapGraph>::make(map) ) {}

struct _ConstRefMapGraph final : ConstGraphInterface {
	using Body = Map<uint32_t,Ref<Set<uint32_t>>>;
private:
	Body const& _body;
public:
	_ConstRefMapGraph( Map<uint32_t,Ref<Set<uint32_t>>>const& org ) : _body(org) {}
	void iter_nodes( NodeFun const& f ) const& override {
		for( auto const& [node,nexts] : _body ) f(node);
	}
	void iter_nexts( uint32_t src, NodeFun const& f ) const& override {
		if( auto const& nexts = _body.find(src) ) {
			for( auto const& next : **nexts ) f(next);
		}
	}
};
ConstGraph::ConstGraph( Map<uint32_t,Ref<Set<uint32_t>>>const& map ) :
	_ptr( Ref<_ConstRefMapGraph>::make(map) ) {}

struct _RefMapConstGraph final : ConstGraphInterface {
	using Body = Map<uint32_t,Ref<Set<uint32_t>>>;
private:
	Body _body;
public:
	_RefMapConstGraph( Map<uint32_t,Ref<Set<uint32_t>>>&& org ) : _body(std::move(org)) {}
	void iter_nodes( NodeFun const& f ) const& override {
		for( auto const& [node,nexts] : _body ) f(node);
	}
	void iter_nexts( uint32_t src, NodeFun const& f ) const& override {
		if( auto const& nexts = _body.find(src) ) {
			for( auto const& next : **nexts ) f(next);
		}
	}
};
ConstGraph::ConstGraph( Map<uint32_t,Ref<Set<uint32_t>>>&& map ) :
	_ptr( Ref<_RefMapConstGraph>::make(std::move(map)) ) {}

struct _AcyclicMapConstRefGraph final : ConstGraphInterface::Acyclic {
	using Body = OrdMap<uint32_t,Set<uint32_t>>;
private:
	Body const& _body;
public:
	_AcyclicMapConstRefGraph( Body const& org ) : _body(org) {}
	void iter_nodes( NodeFun const& f ) const& override {
		for( auto const& [node,nexts] : _body ) f(node);
	}
	void iter_nexts( uint32_t src, NodeFun const& f ) const& override {
		if( auto const& nexts = _body.find(src) ) {
			for( auto const& next : *nexts ) f(next);
		}
	}
};
ConstGraph::Acyclic::Acyclic( OrdMap<uint32_t,Set<uint32_t>>const& map ) :
	_ptr( std::make_unique<_AcyclicMapConstRefGraph>(map) ) {}

struct _AcyclicMapGraph final : ConstGraphInterface::Acyclic {
	using Body = OrdMap<uint32_t,Set<uint32_t>>;
private:
	Body _body;
public:
	_AcyclicMapGraph( Body&& map ) : _body(std::move(map)) {}
	void iter_nodes( NodeFun const& f ) const& override {
		for( auto const& [node,nexts] : _body ) f(node);
	}
	void iter_nexts( uint32_t src, std::function<void(uint32_t)> const& f ) const& override {
		if( auto const& nexts = _body.find(src) ) {
			for( auto const& next : *nexts ) f(next);
		}
	}
};
ConstGraph::Acyclic::Acyclic( OrdMap<uint32_t,Set<uint32_t>>&& map ) :
	_ptr( std::make_unique<_AcyclicMapGraph>(std::move(map)) ) {}

struct _AcyclicVecConstRefGraph final : ConstGraphInterface::Acyclic {
	using Body = std::vector<Set<uint32_t>>;
private:
	Body const& _body;
public:
	_AcyclicVecConstRefGraph( Body const& org ) : _body(org) {}
	void iter_nodes( NodeFun const& f ) const& override {
		for( auto node = 0; node < _body.size(); node++ ) f(node);
	}
	void iter_nexts( uint32_t src, NodeFun const& f ) const& override {
		if( src < _body.size() ) {
			for( auto const& next : _body[src] ) f(next);
		}
	}
};

ConstGraph::Acyclic::Acyclic( std::vector<Set<uint32_t>>const& map ) :
	_ptr( std::make_unique<_AcyclicVecConstRefGraph>(map) ) {}

struct _AcyclicVecGraph final : ConstGraphInterface::Acyclic {
	using Body = std::vector<Set<uint32_t>>;
private:
	Body _body;
public:
	_AcyclicVecGraph( Body&& org ) : _body(std::move(org)) {}
	void iter_nodes( std::function<void(uint32_t)> const& f ) const& override {
		for( auto node = 0; node < _body.size(); node++ ) f(node);
	}
	void iter_nexts( uint32_t src, std::function<void(uint32_t)> const& f ) const& override {
		if( src < _body.size() ) {
			for( auto const& next : _body[src] ) f(next);
		}
	}
};

ConstGraph::Acyclic::Acyclic( std::vector<Set<uint32_t>>&& map ) :
	_ptr( std::make_unique<_AcyclicVecGraph>(std::move(map)) ) {}

struct _MapGraph final : GraphInterface {
	using Body = Map<uint32_t,Set<uint32_t>>;
private:
	Body _body;
public:
	_MapGraph( Map<uint32_t,Set<uint32_t>>&& org ) : _body(std::move(org)) {}
	void iter_nodes( NodeFun const& f ) const& override {
		for( auto const& [node,nexts] : _body ) f(node);
	}
	void iter_nexts( uint32_t src, NodeFun const& f ) const& override {
		if( auto const& nexts = _body.find(src) ) {
			for( auto const& next : *nexts ) f(next);
		}
	}
	bool erase_node( uint32_t node ) & override {
		return _body.erase(node);
	}
	bool erase_edges( uint32_t src, std::function<bool(uint32_t)> const& test ) & override {
		return _body.find(src) && [&]( Set<uint32_t>& nexts )->bool{ return nexts.erase_if(test); };
	}
};
Graph::Graph( Map<uint32_t,Set<uint32_t>>&& map ) :
	_ptr( Ref<_MapGraph>::make(std::move(map)) ) {
}
ConstGraph::ConstGraph( Map<uint32_t,Set<uint32_t>>&& map ) :
	_ptr( Ref<_MapGraph>::make(std::move(map)) ) {
}

static auto const MAX = std::numeric_limits<int>::max();
static auto const MIN = std::numeric_limits<int>::min();

/* An SCC decomposition similar to Pearce's https://doi.org/10.1016/j.ipl.2015.08.010 */
struct _SccMaker {
	ConstGraphInterface const& g;
	/** Node status table
	 * {0..}: represents the node's SCC, which is already closed
	 * {..< 0}: represents the farthest depth the node can reach back
	 */
	Map<uint32_t,int>& table;
	/** Stack of nodes with backlinks */
	std::stack<uint32_t> node_stack;
	/** Stack containing adjacent SCCs */
	std::stack<uint32_t> scc_stack;
	std::vector<ConstGraph::Scc>& sccs;
	std::vector<Set<uint32_t>>& scc_dag;
	int depth;

	int visit( uint32_t u ) {
		auto const [state,fl] = table.emplace(u,depth);
		if( !fl ) {// already visited
			return state;
		}
		bool loop = false, back = false;
		depth--;
		// remember how many nodes were stacked
		uint32_t node_stack_size = node_stack.size();
		uint32_t scc_stack_size = scc_stack.size();
		g.iter_nexts( u, [&]( auto v ){
			auto vstate = visit(v);
			if( vstate >= 0 ) {// v's SCC is known
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
			auto& scc = *sccs.emplace_back(Set<uint32_t>{}).ref<Set<uint32_t>>();
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
		node_stack.emplace(0);
	}
};

ConstGraphInterface::SccInfo const& ConstGraphInterface::scc_info() const& {
	if( _scc_info_opt ) return *_scc_info_opt;
	_scc_info_opt = {{}};
	auto maker = _SccMaker(*this);
	iter_nodes([&]( auto const& node ){
		maker.visit(node);
	});
	return *_scc_info_opt;
}
ConstGraph::Acyclic ConstGraphInterface::Acyclic::acyc_trancl() const {
	OrdMap<uint32_t,Set<uint32_t>> map;
	iter_nodes([&]( auto const& src ){
		auto [nexts,fl] = map.emplace(src,Set<uint32_t>());
		assert(fl);
		iter_nexts(src,[&]( auto const& tgt ){
			for( auto const& next : *ASSERTED(map.find(tgt)) ) {
				nexts.emplace(next);
			}
			nexts.emplace(tgt);
		});
	});
	return std::move(map);
}

Printable print_sccs( std::vector<ConstGraph::Scc> const& sccs ){
	return Printable([&]( ostream& os )->ostream&{
		os << "(sccs";
		for( auto const& scc : sccs ) {
			if( auto triv = scc.ref<uint32_t>() ) {
				os << "\n  " << *triv << flush;
			} else if( auto nodes = scc.ref<Set<uint32_t>>() ){
				os << "\n  (" << print_list(*nodes) << ')' << flush;
			}
		}
		return os << ')' << endl;
	});
}
Printable print_map( auto const& map ){
	return Printable([&]( ostream& os )->ostream&{
		os << '(';
		for( auto const& [k,v] : map ) {
			os << "\n  (" << k << ' ' << v << ')' << flush;
		}
		return os << ')' << endl;
	});	
}

Map<uint32_t,Ref<Set<uint32_t>>> const& ConstGraphInterface::trancl() const& {
	if( !_trancl_opt ) {
		_trancl_opt = {{}};
		auto const& [sccs,scc_inds,scc_dag] = scc_info();
		auto scc_trancl = ConstGraph::Acyclic(scc_dag).acyc_trancl();
		std::vector<Ref<Set<uint32_t>>> scc_reachables;
		scc_trancl.iter_nodes([&]( uint32_t scc )->void{
			auto& reachables = scc_reachables.emplace_back(Ref<Set<uint32_t>>::make());
			scc_trancl.iter_nexts(scc,[&]( uint32_t next_scc ){
				// nodes reachable from the next SCC are reachable
				for( auto const& node : *scc_reachables[next_scc] ) {
					reachables->emplace(node);
				}
				// if next is a trivial SCC, then mark the node reachable, since it is not so from the SCC 
				if( auto const& triv = sccs[next_scc].ref<uint32_t>() ) {
					reachables->emplace(*triv);
				}
			});
			if( auto const& triv = sccs[scc].ref<uint32_t>() ) {// trivial SCC
					_trancl_opt->emplace(*triv,reachables);// the node reaches where the SCC reaches
			} else if( auto const& nodes = sccs[scc].ref<Set<uint32_t>>() ) {
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
Map<uint32_t,Ref<Set<uint32_t>>> ConstGraphInterface::trancl() && {
	return std::move(trancl());
}

Printable ConstGraphInterface::print_nodes( std::string_view const& pref, std::string_view const& _sep ) const& {
	return Printable([&]( std::ostream& os )->std::ostream&{
		std::string_view sep = pref;
		iter_nodes([&]( auto const& src ){
			os << sep << "(node " << src;
			sep = _sep;
			iter_nexts(src,[&]( auto const& tgt ){
				os << ' ' << tgt;
			});
			os << ')' << flush;
		});
		return os;
	});
}

void Graph::test() {
	cerr << "=== Graph ===" << endl;
	auto g = ConstGraph({
		{0,{1}},
		{1,{2}},
		{2,{0,3}},
		{3,{4}},
		{4,{5}},
		{5,{3,6}},
		{6,{7}},
		{7,{6}},
		{8,{7}}
	});
	cout << g << endl;
	cout << print_sccs(g.sccs());
	cout << "(trancl" << ConstGraph(g.trancl()).print_nodes("\n  ","\n  ") << ')' << endl;
	auto g2 = ConstGraph({
		{0,{1}},
		{1,{2}},
		{2,{0,3}},
		{3,{}}
	});
	cout << g2 << endl;
	cout << print_sccs(g2.sccs());
	cout << "(trancl" << ConstGraph(g2.trancl()).print_nodes("\n  ","\n  ") << ')' << endl;
}
