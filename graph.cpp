#include<vector>
#include<limits>
#include<stack>
#include"map.hpp"
#include"graph.hpp"
#include"util.hpp"

using namespace std;

static Set<size_t> const EMPTY = {};

struct _FunGraph final : GraphInterface {
private:
	std::function<void(NodeFun const&)> _node_iter;
	std::function<Set<size_t>const&(size_t)> _nexts;
public:
	_FunGraph(
		std::function<void(NodeFun const&)>&& node_iter,
		std::function<Set<size_t>const&(size_t)>&& nexts ) :
		_node_iter(std::move(node_iter)), _nexts(std::move(nexts)) {}
	void iter_nodes( NodeFun const& f ) const& override {
		_node_iter(f);
	}
	void iter_nexts( size_t src, NodeFun const& f ) const& override {
		for( auto const& next : _nexts(src) ) f(next);
	}
};
Graph::Graph( std::function<void(NodeFun const&)>&& node_iter, std::function<Set<size_t>const&(size_t)>&& nexts ) :
	_ptr(std::make_unique<_FunGraph>(std::move(node_iter),std::move(nexts))) {}

struct _MapConstRefGraph final : GraphInterface {
	using Body = Map<size_t,Set<size_t>>;
private:
	Body const& _body;
public:
	_MapConstRefGraph( Body const& org ) : _body(org) {}
	void iter_nodes( NodeFun const& f ) const& override {
		for( auto const& [node,nexts] : _body ) f(node);
	}
	void iter_nexts( size_t src, NodeFun const& f ) const& override {
		if( auto const& nexts = _body.find(src) ) {
			for( auto const& next : *nexts ) f(next);
		}
	}
};
Graph::Graph( Map<size_t,Set<size_t>>const& map ) :
	_ptr( std::make_unique<_MapConstRefGraph>(map) ) {}

struct _MapGraph final : GraphInterface {
	using Body = Map<size_t,Set<size_t>>;
private:
	Body _body;
public:
	_MapGraph( Map<size_t,Set<size_t>>&& org ) : _body(std::move(org)) {}
	void iter_nodes( NodeFun const& f ) const& override {
		for( auto const& [node,nexts] : _body ) f(node);
	}
	void iter_nexts( size_t src, NodeFun const& f ) const& override {
		if( auto const& nexts = _body.find(src) ) {
			for( auto const& next : *nexts ) f(next);
		}
	}
};
Graph::Graph( Map<size_t,Set<size_t>>&& map ) :
	_ptr( std::make_unique<_MapGraph>(std::move(map)) ) {}


struct _AcyclicMapConstRefGraph final : GraphInterface::Acyclic {
	using Body = OrdMap<size_t,Set<size_t>>;
private:
	Body const& _body;
public:
	_AcyclicMapConstRefGraph( Body const& org ) : _body(org) {}
	void iter_nodes( NodeFun const& f ) const& override {
		for( auto const& [node,nexts] : _body ) f(node);
	}
	void iter_nexts( size_t src, NodeFun const& f ) const& override {
		if( auto const& nexts = _body.find(src) ) {
			for( auto const& next : *nexts ) f(next);
		}
	}
};
Graph::Acyclic::Acyclic( OrdMap<size_t,Set<size_t>>const& map ) :
	_ptr( std::make_unique<_AcyclicMapConstRefGraph>(map) ) {}

struct _AcyclicMapGraph final : GraphInterface::Acyclic {
	using Body = OrdMap<size_t,Set<size_t>>;
private:
	Body _body;
public:
	_AcyclicMapGraph( Body&& map ) : _body(std::move(map)) {}
	void iter_nodes( NodeFun const& f ) const& override {
		for( auto const& [node,nexts] : _body ) f(node);
	}
	void iter_nexts( size_t src, std::function<void(size_t)> const& f ) const& override {
		if( auto const& nexts = _body.find(src) ) {
			for( auto const& next : *nexts ) f(next);
		}
	}
};
Graph::Acyclic::Acyclic( OrdMap<size_t,Set<size_t>>&& map ) :
	_ptr( std::make_unique<_AcyclicMapGraph>(std::move(map)) ) {}

struct _AcyclicVecConstRefGraph final : GraphInterface::Acyclic {
	using Body = std::vector<Set<size_t>>;
private:
	Body const& _body;
public:
	_AcyclicVecConstRefGraph( Body const& org ) : _body(org) {}
	void iter_nodes( NodeFun const& f ) const& override {
		for( auto node = 0; node < _body.size(); node++ ) f(node);
	}
	void iter_nexts( size_t src, NodeFun const& f ) const& override {
		if( src < _body.size() ) {
			for( auto const& next : _body[src] ) f(next);
		}
	}
};

Graph::Acyclic::Acyclic( std::vector<Set<size_t>>const& map ) :
	_ptr( std::make_unique<_AcyclicVecConstRefGraph>(map) ) {}

struct _AcyclicVecGraph final : GraphInterface::Acyclic {
	using Body = std::vector<Set<size_t>>;
private:
	Body _body;
public:
	_AcyclicVecGraph( Body&& org ) : _body(std::move(org)) {}
	void iter_nodes( std::function<void(size_t)> const& f ) const& override {
		for( auto node = 0; node < _body.size(); node++ ) f(node);
	}
	void iter_nexts( size_t src, std::function<void(size_t)> const& f ) const& override {
		if( src < _body.size() ) {
			for( auto const& next : _body[src] ) f(next);
		}
	}
};

Graph::Acyclic::Acyclic( std::vector<Set<size_t>>&& map ) :
	_ptr( std::make_unique<_AcyclicVecGraph>(std::move(map)) ) {}

static auto const MAX = std::numeric_limits<int>::max();
static auto const MIN = std::numeric_limits<int>::min();

/* An SCC decomposition similar to Pearce's https://doi.org/10.1016/j.ipl.2015.08.010 */
struct _SccMaker {
	GraphInterface const& g;
	/** Node status table
	 * {..< 0}: represents the node's depth or the farthest depth it can reach back
	 * {0..}: represents the SCC it constitutes
	 */
	Map<size_t,int>& table;
	/**
	 * {..< 0}: represents a node that constitute the current SCC
	 * {0..<MAX}: represents an SCC that is next to the current SCC
	 * MAX: acyclic
	 */
	std::stack<int> stack;
	std::vector<Set<size_t>>& sccs;
	std::vector<Set<size_t>>& scc_dag;
	int depth;

	int visit( size_t u ) {
		auto const [state,fl] = table.emplace(u,depth);
		if( !fl ) {// already visited
			return state;
		}
		int entrance = depth;
		depth--;
		size_t stack_size = stack.size();// remember how many nodes were stacked
		g.iter_nexts( u, [&]( auto v ){
			auto vstate = visit(v);
			if( vstate < 0 ) {// v has a backlink
				state = std::max(state,vstate);// u can go back as far as v can
			} else if( vstate < MAX ) {// v's SCC is known
				stack.emplace(vstate);
			}
		} );
		if( entrance < state ) {// u constitute a bigger SCC
			stack.emplace( -(int)u - 1 );
			return state;
		}
		// u was the entry point of the SCC
		state = sccs.size();// remember the SCC index
		auto& scc = sccs.emplace_back();
		auto& scc_node = scc_dag.emplace_back();
		scc.emplace(u);
		while( stack.size() != stack_size ) {// things pushed after u belongs to the SCC
			auto top = stack.top();
			stack.pop();
			if( top < 0 ) {// node index
				size_t v = -top - 1;
				scc.emplace(v);
				*ASSERTED(table.find(v)) = state;
			} else if( top < MAX ) {// SCC index
				scc_node.emplace(top);
			}
		}
		return state;
	}
	_SccMaker( GraphInterface const& g ) : depth(-1), g(g),
		sccs(g._scc_info_opt->sccs),
		table(g._scc_info_opt->scc_inds),
		scc_dag(g._scc_info_opt->scc_dag) {
		stack.emplace(0);
	}
};

GraphInterface::SccInfo const& GraphInterface::scc_info() const& {
	if( _scc_info_opt ) return *_scc_info_opt;
	_scc_info_opt = {{}};
	auto maker = _SccMaker(*this);
	iter_nodes([&]( auto const& node ){
		maker.visit(node);
	});
	DEB(Graph::Acyclic(_scc_info_opt->scc_dag));
	return *_scc_info_opt;
}
Graph::Acyclic GraphInterface::Acyclic::acyc_trancl() const {
	OrdMap<size_t,Set<size_t>> map;
	iter_nodes([&]( auto const& src ){
		auto [nexts,fl] = map.emplace(src,Set<size_t>());
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

static std::function<Set<size_t>const&(size_t)> trancl_nexts( GraphInterface const& g ) {
	auto const& [sccs,scc_inds,scc_dag] = g.scc_info();
	auto scc_trancl = Graph::Acyclic(scc_dag).acyc_trancl();
	std::vector<Set<size_t>> scc_reachables;
	scc_trancl.iter_nodes([&]( auto const& scc )->void{
		auto& reachables = scc_reachables.emplace_back();
		scc_trancl.iter_nexts(scc,[&]( const auto& next_scc ){
			for( auto const& node : scc_reachables[next_scc] ) {
				reachables.emplace(node);
			}
		});
		for( auto const& node : sccs[scc] ) {
			reachables.emplace(node);
		}
	});
	return [scc_reachables=std::move(scc_reachables),&scc_inds]( size_t src )->Set<size_t>const&{
		if( auto const& scc_ind = scc_inds.find(src) ) {
			return scc_reachables[*scc_ind];
		}
		return EMPTY;
	};
}
Graph GraphInterface::trancl() const& {
	return Graph( [this]( NodeFun const& f ){ iter_nodes(f); }, trancl_nexts(*this) );
}

Printable GraphInterface::print_nodes( std::string_view const& _pref ) const& {
	return Printable([&]( std::ostream& os )->std::ostream&{
		std::string_view pref = _pref;
		iter_nodes([&]( auto const& src ){
			os << pref << src << " (";
			pref = "\n  (";
			std::string_view pref2 = "";
			iter_nexts(src,[&]( auto const& tgt ){
				os << pref2 << tgt;
				pref2 = " ";
			});
			os << "))" << flush;
		});
		return os;
	});
}

void Graph::test() {
	auto print_sccs = []( auto const& sccs ){
		cout << "SCCs: (";
		for( auto const& scc : sccs ) {
			cout << "\n  (" << print_list(scc) << ')' << flush;
		}
		cout << ')' << endl;
	};
	auto g = Graph({
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
	print_sccs(g.sccs());
	cout << "trancl: " << g.trancl() << endl;
	auto g2 = Graph({
		{0,{1}},
		{1,{2}},
		{2,{0,3}},
		{3,{}}
	});
	cout << g2 << endl;
	print_sccs(g2.sccs());
}
