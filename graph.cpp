#include<vector>
#include<limits>
#include"graph.hpp"
#include"util.hpp"

using namespace std;

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
	Map<size_t,Set<size_t>const&> const& adj;
	size_t visit( size_t u ) {
		auto [info,fl] = table.emplace(u,NodeInfo{});
		if( !fl ) return info.number;// already visited -- report that the caller can go back to u
		info.number = clock;
		info.low = MAX;// `= clock` will and `= MAX` won't count trivial SCCs.
		clock++;
		for( size_t v : *ASSERTED(adj.find(u)) ) {
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
	_SccMaker( Graph const& g ) : clock(0), adj(g.map) {
		ret.emplace_back();// empty slot
		for( auto const& [src,tgts] : adj ) {
			visit(src);
		}
		ret.pop_back();
	}
};

std::vector<Set<size_t>> Graph::sccs() const {
	return std::move(_SccMaker(*this).ret);
}

std::ostream& operator<<( std::ostream& os, Graph const& g ) {
	os << "(graph";
	for( auto const& [src,tgts] : g.map ) {
		os << "\n  (" << src << " (" << print_list(tgts) << "))" << flush;
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
