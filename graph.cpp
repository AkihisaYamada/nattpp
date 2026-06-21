#include<vector>
#include<limits>
#include"map.hpp"
#include"graph.hpp"
#include"util.hpp"

using namespace std;

static Set<size_t> const EMPTY = {};

template<typename M>
auto key_set( M const& map ) {
	Set<typename M::key_type> ret;
	for( auto const& [k,v] : map ) ret.emplace(k);
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

static size_t const MAX = std::numeric_limits<size_t>::max();

/* An SCC decomposition similar to Pearce's https://doi.org/10.1016/j.ipl.2015.08.010 */
struct _SccMaker {
	struct NodeInfo {
		size_t number; // when this node is visited. It will be MAX when it is finished 
		size_t low; // lowest reachable number
	};
	Map<size_t,NodeInfo> table;
	size_t clock;
	std::vector<Set<size_t>> ret;
	Graph const& g;
	size_t visit( size_t u ) {
		auto [info,fl] = table.emplace(u,NodeInfo{});
		if( !fl ) return info.number;// already visited -- report that the caller can go back to u
		info.number = clock;
		info.low = MAX;// `= clock` will and `= MAX` won't count trivial SCCs.
		clock++;
		for( size_t v : g.nexts(u) ) {
			info.low = std::min(info.low,visit(v));// u can go as far as its adjacents can
		}
		if( info.low < info.number ) {// u constitute a member of a broader SCC.
			ret.back().emplace(u);
			return info.low;
		}
		if( info.low == info.number ) {// found SCC
			for( size_t v : ret.back() ) {// mark the SCC components finished
				ASSERTED(table.find(v))->number = MAX;
			}
			ret.back().emplace(u);
			ret.emplace_back();// new slot for next SCC
		}
		return info.number = MAX;
	}
	_SccMaker( Graph const& g ) : clock(0), g(g) {
		ret.emplace_back();// empty slot
		for( auto const& node : g.nodes ) {
			visit(node);
		}
		ret.pop_back();
	}
};

std::vector<Set<size_t>> Graph::sccs() const {
	return std::move(_SccMaker(*this).ret);
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
	print_sccs(g.sccs());
	auto g2 = Graph({
		{0,{1}},
		{1,{2}},
		{2,{0,3}},
		{3,{}}
	});
	print_sccs(g2.sccs());
}
