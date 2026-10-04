#include"poly.hpp"

using namespace std;

ostream& operator<<( ostream& os, Poly::Range const& r ) {
	switch( r ) {
		case Poly::POS: return os << " :range >=0";
		case Poly::NEG: return os << " :range <=0";
		case Poly::FULL: return os;
		default: assert(false);
	}
}

ostream& operator<<( ostream& os, Poly::Var const& v ) {
	return os << "(var " << (string)v << v.range << ')';
}

ostream& operator<<( ostream& os, Poly::Vars const& vs ) {
	os << "(vars"; 
	for( auto const& var : vs.vars() ) {
		os << ' ' << (string)var;
	}
	return os << vs.range() << ')';
}

static ostream& put_monom( ostream& os, pair<Poly::Vars,Smt::PreExp> const& m ) {
	auto const& [vs,c] = m;
	if( vs.range() == Poly::NONE ) return os << c;
	if( c == 1 ) {
		return os << m.first;
	}
	return os << "(* " << c << ' ' << m.first << ')';
}

ostream& operator<<( ostream& os, Poly const& p ) {
	auto const& map = p.map();
	auto const& n = map.size();
	if( n == 0 ) return os << '0';
	auto it = map.begin();
	if( n == 1 ) return put_monom(os,*it);
	auto const& end = map.end();
	os << "(+";
	for( ; it != end; it++ ) {
		put_monom( os << ' ', *it );
	}
	return os << ')';
}
std::ostream& operator<<( std::ostream& os, MPoly const& p ) {
	switch( p.set().size() ) {
	case 0: return os << "-inf";
	case 1: return os << p.set()[0];
	}
	os << "(join";
	for( auto const& x : p.set() ) {
		os << ' ' << x;
	}
	return os << ')';
}

Poly operator+( Poly const& p1, Poly const& p2 ) {
	Poly ret;
	iter2(p1._map,p2._map,[&]( auto const& it1, auto const& it2 ){
		ret._map.emplace( it1->first, it1->second + it2->second );
	},[&]( auto const& it1 ){
		ret._map.emplace(*it1);
	},[&]( auto const& it2 ){
		ret._map.emplace(*it2);
	});
	return std::move(ret);
}
Poly& operator+=( Poly& p1, Poly const& p2 ) {
	iter2(p1._map,p2._map,[&]( auto const it1, auto const it2 ){
		it1->second += it2->second;
	},[&]( auto const it1 ){
	},[&]( auto const it2 ){
		p1._map.emplace(std::move(*it2));
	});
	return p1;
}

Poly ite( Poly const& i, Poly const& p1, Poly const& p2 ) {
	assert( i._map.size() == 1 );
	Smt::PreExp ie = i._map.find({}).value_or_throw(Error("#poly:ite"));
	Poly ret;
	iter2(p1._map,p2._map,[&]( auto const& it1, auto const& it2 ){
		ret._map.emplace( it1->first, Smt::ite(ie,it1->second,it2->second) );
	},[&]( auto const& it1 ){
		ret._map.emplace( it1->first, Smt::ite(ie,it1->second,0) );
	},[&]( auto const& it2 ){
		ret._map.emplace( it2->first, Smt::ite(ie,0,it2->second) );
	});
	return std::move(ret);
}
Poly Poly::monom_mult( Smt::PreExp const& c, Vars const& vs ) const {
	Poly ret;
	for( auto& [vs1,c1] : _map ) {
		ret._map.emplace(vs1 * vs, c * c1);
	}
	return std::move(ret);
}
Poly operator*( Poly const& p1, Poly const& p2 ) {
	Poly ret;
	for( auto const& [vs2,c2] : p2._map ) {
		ret += p1.monom_mult(c2,vs2);
	}
	return std::move(ret);
}

static Smt::PreExp order_sub( Poly const& p1, Poly const& p2 ) {
	Smt::PreExp ge = Smt::TRUE;
	iter2(p1.map(),p2.map(),[&]( auto it1, auto it2 ){
		auto const& e1 = it1->second;
		auto const& e2 = it2->second;
		switch( it1->first.range() ) {
			case Poly::NONE: return;// constant part should be handled by ge/gt
			case Poly::POS: ge = ge && Smt::ge(e1,e2); return;
			case Poly::NEG: ge = ge && Smt::ge(e2,e1); return;
			case Poly::FULL: ge = ge && Smt::eq(e1,e2); return;
		}
	},[&]( auto it1 ){// e1 * vars >= 0
		auto const& e1 = it1->second;
		switch( it1->first.range() ) {
			case Poly::NONE:
			case Poly::POS: ge = ge && Smt::ge(e1,Smt::PostExp(0)); return;
			case Poly::FULL:
			case Poly::NEG: ge = ge && Smt::eq(e1,Smt::PostExp(0)); return;
		}
	},[&]( auto it2 ){// 0 >= e2 * vars
		auto const& e2 = it2->second;
		switch( it2->first.range() ) {
			case Poly::NONE:
			case Poly::NEG: ge = ge && Smt::le(Smt::PostExp(0),e2); return;
			case Poly::FULL:
			case Poly::POS: ge = ge && Smt::eq(Smt::PostExp(0),e2); return;
		}
	});
	return ge;
}

Smt::PreExp Poly::ge( Poly const& p2 ) const {
	return order_sub(*this,p2) && Smt::ge((*this)[{}],p2[{}]);
}
Smt::Compare order( Poly const& p1, Poly const& p2, Smt::Solver& solver ) {
	auto const& val = solver.let(Smt::BOOL,order_sub(p1,p2));
	auto const& c1 = solver.expand(p1[{}]);
	auto const& c2 = solver.expand(p2[{}]);
	return { val && Smt::ge(c1,c2), val && Smt::gt(c1,c2) };
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
	if( auto cond = f.ref<Poly::Cond>() ) {
		return os << "(#ite " << cond->exp << ')';
	}
	if( auto e = f.ref<Smt::PreExp>() ) {
		return os << *e;
	}
	assert(false);
}

Algebra<Template::Sym,Poly> Poly::algebra_of_range( Poly::Range ran ) {
	return [ran]( Template::Sym const& f, std::vector<Poly> const& args )->Poly {
		if( auto const& smt = f.is_smt() ) return *smt;
		if( auto const& sym = f.is_fun() ) {
			if( sym->name == Smt::ADD ) return sum(args);
			if( sym->name == Smt::MUL ) return prod(args);
			if( sym->name == Smt::ITE ) {
				assert( args.size() == 3 );
				return ite(args[0],args[1],args[2]);
			}
			throw Error("#poly","\"unknown fun\"",sym->name);
		}
		if( ran != NONE ) {
			if( auto const& var = f.is_var() ) return Var(*var,ran);
		}
		assert(false);
	};
}
Algebra<Template::Sym,Poly> const Poly::ALGEBRA = algebra_of_range(NONE);
Algebra<Template::Sym,Poly> const Poly::POS_ALGEBRA = algebra_of_range(POS);

Algebra<Template::Sym,MPoly> MPoly::algebra_of_range( Poly::Range ran ) {
	return [ran]( Template::Sym const& f, std::vector<MPoly> const& args )->MPoly {
		if( auto const& smt = f.is_smt() ) return *smt;
		if( auto const& sym = f.is_fun() ) {
			if( sym->name == Smt::ADD ) return sum(args);
			if( sym->name == Smt::MUL ) return prod(args);
			if( sym->name == Smt::ITE ) {
				assert( args.size() == 3 );
				return ite(args[0],args[1],args[2]);
			} 
			if( sym->name == Smt::MAX ) {
				return chain(MPoly(),(MPoly&(*)(MPoly&,MPoly const&))max_eq,args);
			}
			throw Error("#mpoly","\"unknown fun\"",sym->name);
		}
		if( ran != Poly::NONE ) {
			if( auto const& var = f.is_var() ) return Poly::Var(*var,ran);
		}
		assert(false);
	};
}
Algebra<Template::Sym,MPoly> const MPoly::ALGEBRA = algebra_of_range(Poly::NONE);
Algebra<Template::Sym,MPoly> const MPoly::POS_ALGEBRA = algebra_of_range(Poly::POS);

MPoly operator+( MPoly const& x, MPoly const& y ) {
	MPoly ret;
	for( auto const& xp : x._set ) {
		for( auto const& yp : y._set ) {
			ret._set.emplace_back( xp + yp );
		}
	}
	return std::move(ret);
}

MPoly operator*( MPoly const& x, MPoly const& y ) {
	MPoly ret;
	for( auto const& xp : x._set ) {
		for( auto const& yp : y._set ) {
			ret._set.emplace_back(xp*yp);
		}
	}
	return std::move(ret);
}
MPoly ite( MPoly const& cm, MPoly const& tm, MPoly const& em ) {
	assert( cm.set().size() == 1 );
	auto const& cp = cm.set()[0];
	MPoly ret;
	for( auto const& tp : tm.set() ) {
		for( auto const& ep : em.set() ) {
			ret._set.emplace_back(ite(cp,tp,ep));
		}
	}
	return std::move(ret);
}
Smt::PreExp MPoly::ge( MPoly const& y ) const {
	Smt::PreExp ge_all = Smt::PostExp(true);
	for( auto const& yp : y._set ){
		Smt::PreExp some_ge = Smt::PostExp(false);
		for( auto const& xp : _set ){
			some_ge = some_ge || xp.ge(yp);
		}
		ge_all = ge_all && some_ge;
	}
	return ge_all;
}

Smt::Compare order( MPoly const& x, MPoly const& y, Smt::Solver& solver ) {
	Smt::PostExp ge_all = true, gt_all = true;
	for( auto const& yp : y._set ){
		Smt::PostExp some_ge = false, some_gt = false;
		for( auto const& xp : x._set ){
			auto [ge,gt] = order(xp,yp,solver);
			some_ge = some_ge || ge;
			some_gt = some_gt || gt;
		}
		ge_all = ge_all && some_ge;
		gt_all = gt_all && some_gt;
	}
	return {ge_all,gt_all};
}

int Poly::test() {
	cout << "=== Poly test ===" << endl;
	auto subst = Subst<string>{{"x",Exp{"g","y"}}};
	cout << subst(Exp{"f","x"}) << endl;
	auto subst2 = Subst<string>{{"x",Exp{"g","x"}}};
	cout << subst2(Exp{"f","x","x"}) << endl;
	auto z3 = Smt::Z3(Smt::QF_LIA);
	auto c1 = z3.declare_const("c1",Smt::INT);
	auto c2 = z3.declare_const("c2",Smt::INT);
	auto wa = z3.declare_const("wa",Smt::INT);
	auto wb = z3.declare_const("wb",Smt::INT);
	auto hsubst = Deriver<string,Template::Sym>{
		{ "f", Term<Sum<Template::Sym,Arg>>(
			Template::Fun("+"),
			Term<Sum<Template::Sym,Arg>>(Template::Fun("*"),c1,Arg(0)),
			Term<Sum<Template::Sym,Arg>>(Template::Fun("*"),c2,Arg(1))
		)},
		{ "a", Term<Sum<Template::Sym,Arg>>(Template::Fun("+"),Arg(0),wa) },
		{ "b", Term<Sum<Template::Sym,Arg>>(Template::Fun("+"),Arg(0),wb) }
	};
	auto e = Exp{"f",Exp{"a","x"},Exp{"b","x"}};
	cout << "⟦" << e << "⟧ = " << hsubst.derive(TERM<Template::Sym>)(e) << endl;
	cout << hsubst.derive(MPoly::ALGEBRA)(e) << endl;
	Poly x = Poly::Var("x",Poly::POS);
	Poly y = Poly::Var("y",Poly::NEG);
	auto c = z3.declare_const("c",Smt::INT);
	auto d = z3.declare_const("d",Smt::INT);
	Poly p = x * c + d, q = x * 5 + 3;
	cout << "Poly: " << p << " >= " << q << endl;
	cout << "Smt: " << z3.expand(p.ge(q)) << endl;

	return 0;
}
