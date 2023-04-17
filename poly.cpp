#include"poly.hpp"

using namespace std;

static ostream& put_monom( ostream& os, pair<Poly::Vars,Smt::Exp> const& m ) {
	os << m.second;
	for( auto const& var : m.first.vars() ) {
		os << " * " << var;
	}
	return os;
}
ostream& operator<<( ostream& os, Poly const& p ) {
	auto const& map = p.map();
	auto it = map.begin(), end = map.end();
	if( it == end ) {
		return os << '0';
	}
	put_monom(os,*it);
	it++;
	for( ; it != end; it++ ) {
		os << " + ";
		put_monom(os,*it);
	}
	return os;
}
int Poly::test() {
	Subst subst = {{"x",{"g","y"}}};
	cout << subst.apply({"f","x"}) << endl;
	Subst subst2 = {{"x",{"g","x"}}};
	cout << subst2.apply({"f","x","x"}) << endl;
	Subst hsubst = {
		{"f",{"+",{"*","c1",{":in",0}},{"*","c2",{":in",1}}}},
		{"a",{"+",{":in",0},"wa"}},
		{"b",{"+",{":in",0},"wb"}},
	};
	Exp e = {"f",{"a","x"},{"b","x"}};
	cout << e << hsubst << " = " << hsubst.apply(e) << endl;
	cout << hsubst.derive(Poly::ALGEBRA).eval(e) << endl;
	Poly x = Poly::Var("x",Poly::POS);
	Poly y = Poly::Var("y",Poly::NEG);
	cout << (x + 5) * (y * 3 + 2) << endl;
	cout << (x * Smt::Exp("c") + Smt::Exp("d")).ge(x * 5 + 3) << endl;
	return 0;
}
