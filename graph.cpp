#include<vector>
#include<limits>
#include<stack>
#include"map.hpp"
#include"sum.hpp"
#include"graph.hpp"
#include"util.hpp"

using namespace std;


Printable print_map( auto const& map ){
	return Printable([&]( ostream& os )->ostream&{
		os << '(';
		for( auto const& [k,v] : map ) {
			os << "\n  (" << k << ' ' << v << ')' << flush;
		}
		return os << ')' << endl;
	});	
}

void graph_test() {
	cerr << "=== Graph ===" << endl;
	auto g = ConstGraph<int>({
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
	cout << print_sccs<int>(g.sccs());
	cout << "(trancl" << ConstGraph<int>(g.trancl()).print_nodes("\n  ","\n  ") << ')' << endl;
	auto g2 = ConstGraph<int>({
		{0,{1}},
		{1,{2}},
		{2,{0,3}},
		{3,{}}
	});
	cout << g2 << endl;
	cout << print_sccs<int>(g2.sccs());
	cout << "(trancl" << ConstGraph<int>(g2.trancl()).print_nodes("\n  ","\n  ") << ')' << endl;
}
