#include"poly.hpp"

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
Poly Poly::monom_mult( Smt::PreExp const& c, Vars const& vs ) const {
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

static Smt::PreExp order_sub( Poly const& p1, Poly const& p2 ) {
	Smt::PreExp ge = Smt::TRUE;
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
			ge = ge && Smt::eq(it1->second,Smt::PreExp(0));
		}
	},[&]( auto it2 ){
		switch( it2->first.range() ) {
			case Poly::NEG: case Poly::FULL:
			ge = ge && Smt::eq(it2->second,Smt::PreExp(0));
		}
	});
	return ge;
}

Smt::PreExp Poly::ge( Poly const& p2 ) const {
	return order_sub(*this,p2) && Smt::ge((*this)[{}],p2[{}]);
}
Smt::PreExp Poly::order( Poly const& p1, Poly const& p2, Smt::Solver& solver ) {
	auto const& val = solver.let(Smt::BOOL,order_sub(p1,p2));
	auto const& c1 = p1[{}];
	auto const& c2 = p2[{}];
	return ( val && Smt::ge(c1,c2), val && Smt::gt(c1,c2) );
}

Poly& Poly::memoize( Smt::Solver& solver, Smt::BaseSort const& sort ) {
	for( auto& val : _map ) {
		if( val.first.vars().empty() ) {
			val.second = solver.let(sort,val.second);
		} else {
			val.second = solver.expand(val.second);
		}
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
	if( auto e = f.ref<Smt::PreExp>() ) {
		return os << *e;
	}
	assert(false);
}

Algebra::Intp<Poly::Sig,Poly> Poly::algebra( Smt::Solver& solver ) {
	return [&solver]( Poly::Sig const& f, std::vector<Poly> const& args )->Poly{
		if( f.ref<Add>() ) {
			return sum(args);
		}
		if( f.ref<Mul>() ) {
			return prod(args);
		}
		assert( args.size() == 0 );
		if( auto var = f.ref<Var>() ) {
			return Var(*var,Poly::POS);
		}
		if( auto c = f.ref<Smt::PreExp>() ) {
			return *c;
		}
		assert(false);
	};
}

template<typename T1, typename T2>
ostream& operator<<( ostream& os, pair<T1,T2> const& pair ) {
	return os << "〈" << pair.first << ", " << pair.second << "〉";
}

int Poly::test() {
	cout << "=== Poly test ===" << endl;
	Subst<string> subst = {{"x",Exp{"g","y"}}};
	cout << subst.eval(Exp{"f","x"}) << endl;
	Subst<string> subst2 = {{"x",Exp{"g","x"}}};
	cout << subst2.eval(Exp{"f","x","x"}) << endl;
	auto z3 = Smt::Z3(Smt::QF_LIA);
	auto c1 = z3.declare_const("c1",Smt::INT);
	auto c2 = z3.declare_const("c2",Smt::INT);
	auto wa = z3.declare_const("wa",Smt::INT);
	auto wb = z3.declare_const("wb",Smt::INT);
	Algebra::Deriver<string,Sig> hsubst = [&]( string const& f ){
		if( f == "f" ) {
			return Term<Sum<Sig,Algebra::Arg>>{
				ADD,
				Term<Sum<Sig,Algebra::Arg>>{MUL,c1,Algebra::Arg(0)},
				Term<Sum<Sig,Algebra::Arg>>{MUL,c2,Algebra::Arg(1)}
			};
		}
		if( f == "a" ) {
			return Term<Sum<Sig,Algebra::Arg>>{ADD,Algebra::Arg(0),wa};
		}
		if( f == "b" ) {
			return Term<Sum<Sig,Algebra::Arg>>{ADD,Algebra::Arg(0),wb};
		}
		return Term<Sum<Sig,Algebra::Arg>>(Poly::Var(f,POS));
	};
	auto e = Exp{"f",Exp{"a","x"},Exp{"b","x"}};
	cout << e << " = " << hsubst.derive(Algebra::TERM<Sig>).eval(e) << endl;
	auto z3poly = Poly::algebra(z3);
	cout << hsubst.derive(z3poly).eval(e) << endl;
	Poly x = Poly::Var("x",Poly::POS);
	Poly y = Poly::Var("y",Poly::NEG);
	auto c = z3.declare_const("c",Smt::INT);
	auto d = z3.declare_const("d",Smt::INT);
	Poly p = x * c + d, q = x * 5 + 3;
	cout << "Poly: " << p << " <= " << q << endl;
	cout << "Smt: " << z3.expand(p.ge(q)) << endl;
	Trs::Sig sig;
	sig.insert("f",2);
	sig.insert("g",1);
	sig.insert("a",0);
	auto der = Template::SUM.deriver(sig,z3);
	auto der_intp = memoize(der.derive(z3poly),z3,Smt::INT);
	e = Exp{"f",Exp{"g","x"},"a"};
	for( auto p : sig ) {
		cout << "der(" << p.first << ") = " << der(p.first) << endl;
	}
	auto der_term = der.derive(Algebra::TERM<Sig>);
	cout << "der⟦" << "(g x)" << "⟧ = " << der_term.eval(Exp{"g","x"}) << endl;
	cout << "der⟦a⟧ = " << der_term.eval("a") << endl;
	cout << "der⟦" << e << "⟧ = " << der_term.eval(e) << endl;
	cout << "Poly: " << der_intp.eval(e) << endl;
	cout << "Annotate: " << der_intp.annotate(e) << endl;
	return 0;
}
