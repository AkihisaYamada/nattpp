#include<vector>
#include<stack>
#include<limits>
#include"graph.hpp"
#include"util.hpp"

using namespace std;

static size_t const MAX = std::numeric_limits<size_t>::max();

/* A potentially novel variant of Tarjan's SCC algorithm. */
struct _SccMaker {
	struct NodeInfo {
		size_t number; // when this node is visited. It will be MAX when it is finished 
		size_t low; // lowest reachable number
	};
	Map<size_t,NodeInfo> table;
	size_t clock;
	Set<size_t> acc;
	std::vector<Set<size_t>> ret;
	Map<size_t,Set<size_t>> const& adj;
	size_t visit( size_t u ) {
		auto [info,fl] = table.emplace(u,NodeInfo{});
		if( !fl ) return info.number;// already visited -- report that the caller can go back to u
		info.number = clock;
		info.low = clock;// `= clock` will and `= MAX` won't count trivial SCCs.
		clock++;
		for( size_t v : *ASSERTED(adj.find(u)) ) {
			info.low = std::min(info.low,visit(v));// u can go as far as its adjacents can
		}
		if( info.low < info.number ) {// u constitute a member of a broader SCC.
			acc.emplace(u);
			return info.low;
		}
		if( info.low == info.number ) {// found SCC
			for( size_t v : acc ) {// mark scc components acyclic
				ASSERTED(table.find(v))->number = MAX;
			}
			acc.emplace(u);
			ret.emplace_back(std::move(acc));
		}
		return info.number = MAX;
	}
	_SccMaker( Graph const& g ) : clock(0), adj(g.map) {
		for( auto const& [src,tgts] : adj ) {
			visit(src);
		}
	}
};

std::vector<Set<size_t>> Graph::sccs() const {
	return std::move(_SccMaker(*this).ret);
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
