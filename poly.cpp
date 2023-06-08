#include"poly.hpp"
#include"template.hpp"

using namespace std;

static ostream& put_monom( ostream& os, pair<Poly::Vars,Smt::PreExp> const& m ) {
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
Poly Poly::operator+( Poly const& p2 ) const & {
	Poly ret;
	iter2(_map,p2._map,[&]( auto& it1, auto& it2 ){
		ret._map.insert( it1->first, it1->second + it2->second );
	},[&]( auto& it1 ){
		ret._map.insert(*it1);
	},[&]( auto& it2 ){
		ret._map.insert(*it2);
	});
	return std::move(ret);
}
Poly& Poly::operator+=( Poly const& p2 ) & {
	iter2(_map,p2._map,[&]( auto it1, auto it2 ){
		it1->second += it2->second;
	},[&]( auto it1 ){
	},[&]( auto it2 ){
		_map.insert(*it2);
	});
	return *this;
}
Poly Poly::monom_mult( Smt::PostExp const& c, Vars const& vs ) const {
	Poly ret;
	for( auto& [vs1,c1] : _map ) {
		ret._map.insert(vs1 * vs, c * c1);
	}
	return std::move(ret);
}
Poly Poly::operator*( Poly const& p2 ) const {
	Poly ret;
	for( auto const& [vs2,c2] : p2._map ) {
		ret += monom_mult(c2,vs2);
	}
	return std::move(ret);
}

static Smt::PostExp order_sub( Poly const& p1, Poly const& p2 ) {
	Smt::PostExp ge = Smt::TRUE;
	iter2(p1.map(),p2.map(),[&]( auto it1, auto it2 ){
		switch( it1->first.range() ) {
			case Poly::NONE: return;
			case Poly::POS: ge = ge && Smt::ge(it1->second,it2->second); return;
			case Poly::NEG: ge = ge && Smt::ge(it2->second,it1->second); return;
			case Poly::FULL: ge = ge && Smt::eq(it1->second,it2->second); return;
		}
	},[&]( auto it1 ){
		switch( it1->first.range() ) {
			case Poly::NEG: case Poly::FULL:
			ge = ge && Smt::eq(it1->second,0);
		}
	},[&]( auto it2 ){
		switch( it2->first.range() ) {
			case Poly::NEG: case Poly::FULL:
			ge = ge && Smt::eq(it2->second,0);
		}
	});
	return ge;
}

Smt::PostExp Poly::ge( Poly const& p2 ) const {
	return order_sub(*this,p2) && Smt::ge((*this)[{}],p2[{}]);
}
Smt::PostExp Poly::order( Poly const& p1, Poly const& p2, Smt::Solver& solver ) {
	auto const& val = solver.let(Smt::BOOL,order_sub(p1,p2));
	auto const& c1 = p1[{}];
	auto const& c2 = p2[{}];
	return ( val && Smt::ge(c1,c2), val && Smt::gt(c1,c2) );
}

Poly& Poly::memoize( Smt::Solver& solver, Smt::BaseSort const& sort ) {
	for( auto& [key,val] : _map ) {
		val = solver.let(sort,val);
	}
	return *this;
}

ostream& operator<<( ostream& os, Poly::Sig const& f ) {
	if( f.ref<Poly::Add>() ) {
		return os << '+';
	}
	if( f.ref<Poly::Mul>() ) {
		return os << '*';
	}
	if( auto var = f.ref<Poly::Var>() ) {
		return os << *var;
	}
	if( auto e = f.ref<Smt::PostExp>() ) {
		return os << *e;
	}
	assert(false);
}

Algebra::Intp<Poly::Sig,Poly> Poly::algebra( Smt::Solver& solver, Smt::BaseSort const& sort ) {
	return [&solver,sort]( Poly::Sig const& f, std::vector<Poly> const& args )->Poly{
		if( f.ref<Add>() ) {
			return sum(args).memoize(solver,sort);
		}
		if( f.ref<Mul>() ) {
			return prod(args).memoize(solver,sort);
		}
		assert( args.size() == 0 );
		if( auto var = f.ref<Var>() ) {
			return Var(*var,Poly::POS);
		}
		if( auto c = f.ref<Smt::PostExp>() ) {
			return *c;
		}
		assert(false);
	};
}


int Poly::test() {
	cout << "=== Poly test ===" << endl;
	Subst<string> subst = {{"x",Exp{"g","y"}}};
	cout << subst.eval(Exp{"f","x"}) << endl;
	Subst<string> subst2 = {{"x",Exp{"g","x"}}};
	cout << subst2.eval(Exp{"f","x","x"}) << endl;
	auto z3 = Smt::Z3(Smt::QF_LIA,cout);
	auto c1 = z3.declare_const("c1",Smt::INT);
	auto c2 = z3.declare_const("c2",Smt::INT);
	auto wa = z3.declare_const("wa",Smt::INT);
	auto wb = z3.declare_const("wb",Smt::INT);
	Algebra::Deriver<string,Sig> hsubst = [&]( string const& f ){
		if( f == "f" ) {
			return Tree<Sum<Sig,Algebra::Arg>>{
				ADD,
				Tree<Sum<Sig,Algebra::Arg>>{MUL,c1,Algebra::Arg(0)},
				Tree<Sum<Sig,Algebra::Arg>>{MUL,c2,Algebra::Arg(1)}
			};
		}
		if( f == "a" ) {
			return Tree<Sum<Sig,Algebra::Arg>>{ADD,Algebra::Arg(0),wa};
		}
		if( f == "b" ) {
			return Tree<Sum<Sig,Algebra::Arg>>{ADD,Algebra::Arg(0),wb};
		}
		return Tree<Sum<Sig,Algebra::Arg>>(Poly::Var(f,POS));
	};
	auto e = Exp{"f",Exp{"a","x"},Exp{"b","x"}};
	cout << e << " = " << hsubst.derive(Algebra::TERM<Sig>).eval(e) << endl;
	auto z3poly = Poly::algebra(z3,Smt::INT);
	cout << hsubst.derive(z3poly).eval(e) << endl;
	Poly x = Poly::Var("x",Poly::POS);
	Poly y = Poly::Var("y",Poly::NEG);
	auto c = z3.declare_const("c",Smt::INT);
	auto d = z3.declare_const("d",Smt::INT);
	cout << (x + 5) * (y * 3 + 2) << endl;
	cout << (x * c + d).ge(x * 5 + 3) << endl;
	Trs::Sig sig;
	sig.insert("f",2);
	sig.insert("g",1);
	sig.insert("a",0);
	auto der = Template::SUM.deriver(sig,z3);
	e = Exp{"f",Exp{"g","x"},"a"};
	cout << der.derive(Algebra::TERM<Sig>).eval(e) << endl;
	cout << der.derive(z3poly).eval(e) << endl;
	return 0;
}
