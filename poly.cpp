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

Algebra::Intp<Poly> const Poly::VAR_INTP = Algebra::Intp<Poly>(
	[]( std::string_view const& f, std::vector<Poly> const& args )->Poly{
		if( !args.empty() ) {
			throw Poly::Error("#var-intp");
		}
		return Poly::Var(f,Poly::POS);
	}
);

Poly& Poly::memoize( Smt::Solver& solver, Smt::BaseSort const& sort ) {
	for( auto& [key,val] : _map ) {
		val = solver.let(sort,val);
	}
	return *this;
}

Algebra::Intp<Poly> Poly::algebra( Smt::Solver& solver, Smt::BaseSort const& sort ) {
	return [&solver,sort]( std::string_view const& f, std::vector<Poly> const& args )->Poly{
		if( f == "+" ) {
			return sum(args).memoize(solver,sort);
		}
		if( f == "*" ) {
			return prod(args).memoize(solver,sort);
		}
		if( f == "ite" ) {
			if( args.size() != 3 ) {
				throw Poly::Error{"#arity-mismatch",f};
			}
			auto const& i = args[0].map(), & t = args[1].map(), & e = args[2].map();
			if( i.size() != 1 || t.size() != 1 || e.size() != 1 ) {
				throw Poly::Error{"#ite-poly"};
			}
			auto vi = i.find({}), vt = t.find({}), ve = e.find({});
			if( !vi || !vt || !ve ) {
				throw Poly::Error{"#ite-poly"};
			}
			return Smt::If(*vi) ^ *vt ^ *ve;
		}
		assert( args.size() == 0 );
		return Smt::PostExp(f);
	};
}


int Poly::test() {
	cout << "=== Poly test ===" << endl;
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
	auto z3 = Smt::Z3(Smt::QF_LIA,cout);
	auto z3alg = Poly::algebra(z3,Smt::INT);
	cout << hsubst.derive(z3alg,Poly::VAR_INTP).eval(e) << endl;
	Poly x = Poly::Var("x",Poly::POS);
	Poly y = Poly::Var("y",Poly::NEG);
	cout << (x + 5) * (y * 3 + 2) << endl;
	cout << (x * Smt::PostExp("c") + Smt::PostExp("d")).ge(x * 5 + 3) << endl;
	Trs::Sig sig;
	sig.insert("f",2);
	sig.insert("g",1);
	sig.insert("a",0);
	auto der = Template::SUM.deriver(sig,z3);
	e = Exp{"f",Exp{"g","x"},"a"};
	cout << der << e << endl;
	cout << der.subst(e) << endl;
	cout << der.derive(z3alg,Poly::VAR_INTP).eval(e) << endl;
	return 0;
}
