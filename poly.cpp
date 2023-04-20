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


Algebra::Intp<Poly> const Poly::ALGEBRA = Algebra::Intp<Poly>(
	[]( std::string_view const& f, std::vector<Poly> const& args )->Poly{
		if( f == "+" ) {
			return sum(args);
		}
		if( f == "*" ) {
			return prod(args);
		}
		if( f == "ite" ) {
			if( args.size() != 3 ) {
				throw Algebra::Error{"#arity-mismatch",f};
			}
			auto const& i = args[0].map(), & t = args[1].map(), & e = args[2].map();
			if( i.size() != 1 || t.size() != 1 || e.size() != 1 ) {
				throw Algebra::Error{"#ite-poly"};
			}
			auto vi = i.find({}), vt = t.find({}), ve = e.find({});
			if( !vi || !vt || !ve ) {
				throw Algebra::Error{"#ite-poly"};
			}
			return Smt::Exp(f,{*vi,*vt,*ve});
		}
		assert( args.size() == 0 );
		return Smt::Exp(f);
	}
);

int Poly::test() {
	Deriver::Map subst = {{"x",Exp{"g","y"}}};
	cout << subst.subst(Exp{"f","x"}) << endl;
	Deriver::Map subst2 = {{"x",Exp{"g","x"}}};
	cout << subst2.subst(Exp{"f","x","x"}) << endl;
	Deriver::Map hsubst = {
		{"f",Exp{"+",Exp{"*","c1",Exp{":in",0}},Exp{"*","c2",Exp{":in",1}}}},
		{"a",Exp{"+",Exp{":in",0},"wa"}},
		{"b",Exp{"+",Exp{":in",0},"wb"}},
	};
	auto e = Exp{"f",Exp{"a","x"},Exp{"b","x"}};
	cout << e << hsubst << " = " << hsubst.subst(e) << endl;
	cout << hsubst.derive(Poly::ALGEBRA).eval(e) << endl;
	Poly x = Poly::Var("x",Poly::POS);
	Poly y = Poly::Var("y",Poly::NEG);
	cout << (x + 5) * (y * 3 + 2) << endl;
	cout << (x * Smt::Exp("c") + Smt::Exp("d")).ge(x * 5 + 3) << endl;

	Deriver::Template temp = Exp{
		"arity",
		Exp{"0",Exp{"var","int"}},
		Exp{"1",Exp{"+",Exp{"var","int"},Exp{"*",Exp{"var","int"},"arg"}}},
		Exp{"t",Exp{"+",Exp{"args","+",Exp{"*",Exp{"ite",Exp{"var","bool"},1,0},"arg"}},Exp{"var","int"}}}
	};
	cout << temp << endl;
	Trs::Sig sig;
	sig.insert("f",2);
	sig.insert("g",1);
	sig.insert("a",0);
	auto z3 = Smt::Z3(cout);
	auto der = temp.deriver(sig,z3);
	cout << der.subst(Exp{"f",Exp{"g","x"},"a"}) << endl;
	cout << der.derive(Poly::ALGEBRA).eval(Exp{"f",Exp{"g","x"},"a"}) << endl;
	return 0;
}
