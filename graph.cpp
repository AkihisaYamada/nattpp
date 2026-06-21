#include<vector>
#include<limits>
#include<stack>
#include"map.hpp"
#include"graph.hpp"
#include"util.hpp"

using namespace std;

static Set<size_t> const EMPTY = {};

template<typename M>
auto key_set( M const& map ) {
	Set<typename M::key_type> ret;
	for( auto const& [k,v] : map ) {
		ret.emplace(k);
	}
	return std::move(ret);
}

Graph::Graph( Map<size_t,Set<size_t>>const& map ) :
	nodes(key_set(map)),
	_fun( [&map]( size_t src )->auto&{ return map.find(src).value_or(EMPTY); } )
{}

Graph::Graph( Map<size_t,Set<size_t>>&& map ) :
	nodes(key_set(map)),
	_fun( [map=std::move(map)]( size_t src )->auto&{ return map.find(src).value_or(EMPTY); } )
{}

static int const MAX = std::numeric_limits<int>::max();
static int const MIN = std::numeric_limits<int>::min();

/* An SCC decomposition similar to Pearce's https://doi.org/10.1016/j.ipl.2015.08.010 */
struct _SccMaker {
	Graph const& g;
	/** States of nodes.
	 * < 0: node is open, and holds the negated lowest reachable depth
	 * >= 0: node is closed, and holds the index of the SCC 
	 */
	Map<size_t,int> table;
	int depth;
	std::stack<size_t> stack;
	std::vector<Set<size_t>> sccs;
	int visit( size_t u ) {
		auto const [state,fl] = table.emplace(u,depth);
		if( !fl ) {// already visited
			return state;
		}
		depth--;
		int low = MIN;
		stack.emplace(u);
		for( size_t v : g.nexts(u) ) {
			int vstate = visit(v);
			if( vstate < 0 ) {// v has a backlink
				low = std::max(low,vstate);// u can go back as far as v can
			} else {// v is closed
			}
		}
		if( low < state ) {// u is acyclic
			stack.pop();
			return state = MIN;// mark u closed
		}
		if( state < low ) {// u constitute a bigger SCC
			return state = low;
		}
		state = sccs.size();// remember the SCC index
		auto& scc = sccs.emplace_back();
		for(;;) {
			auto v = stack.top();
			stack.pop();
			scc.emplace(v);
			if( v == u ) return state;
			*ASSERTED(table.find(v)) = state;
		}
	}
	_SccMaker( Graph const& g ) : depth(-1), g(g) {
		stack.emplace(0);
	}
};

std::vector<Set<size_t>> Graph::sccs() const {
	auto maker = _SccMaker(*this);
	for( auto const& node : nodes ) {
		maker.visit(node);
	}
	return std::move(maker.sccs);
}

std::ostream& operator<<( std::ostream& os, Graph const& g ) {
	os << "(graph";
	for( auto const& src : g.nodes ) {
		os << "\n  (" << src << " (" << print_list(g.nexts(src)) << "))" << flush;
	}
	return os << ')';
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
	auto g2 = Graph({
		{0,{1}},
		{1,{2}},
		{2,{0,3}},
		{3,{}}
	});
	cout << g2 << endl;
	print_sccs(g2.sccs());
}
