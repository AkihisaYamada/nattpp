/* Tarjan's SCC algorithm.
 */
#include <climits>
#include<vector>
#include<stack>
#include"graph.hpp"
#include"util.hpp"

using namespace std;

struct _SccMaker {
	struct NodeInfo {
		size_t number; // when this node is visited. UINT_MAX when it is finished 
		size_t low; // lowest reachable number
	};
	Map<size_t,NodeInfo> table;
	size_t clock;
	std::stack<size_t> stack;
	std::vector<Set<size_t>> ret;
	Map<size_t,Set<size_t>> const& adj;
	size_t visit( size_t u ) {
		auto [info,fl] = table.emplace(u,NodeInfo{});
		if( !fl ) return info.number;// already visited - return where you can go back
		info.number = info.low = clock;
		clock++;
		stack.push(u);
		for( size_t v : *ASSERTED(adj.find(u)) ) {
			info.low = std::min(info.low,visit(v));
		}
		if( info.low == info.number ) {// an SCC identified
			auto& scc = ret.emplace_back();
			for(;;) {
				size_t v = stack.top();
				scc.emplace(v);
				stack.pop();
				ASSERTED(table.find(v))->number = UINT_MAX;
				if( v == u ) break;
			}
		}
		return info.low;
	}
	_SccMaker( Graph const& g ) : clock(1), adj(g.map) {
		for( auto const& [src,tgts] : adj ) {
			visit(src);
		}
	}
};

std::vector<Set<size_t>> Graph::sccs() const {
	return std::move(_SccMaker(*this).ret);
}

void Graph::test() {
	auto g = Graph({
		{0, {1}},
		{1, {2}},
		{2, {0, 3}},
		{3, {4}},
		{4, {5}},
		{5, {3, 6}},
		{6, {7}},
		{7, {6}},
		{8, {7}}
	});
	auto sccs = g.sccs();
	cout << "SCCs: (";
	for( auto const& scc : sccs ) {
		cout << "\n  (" << print_list(scc) << ')' << flush;
	}
	cout << ')' << endl;
}
