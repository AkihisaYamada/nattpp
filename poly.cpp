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
		p1._map.emplace(*it2);
	});
	return p1;
}

Poly Poly::ite( Poly const& i, Poly const& p1, Poly const& p2 ) {
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
			case Poly::NONE: return;
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

Poly& Poly::expand( Smt::Solver& solver ) {
	for( auto& [vars,coeff] : _map ) {
		if( !solver.logic().linear() || vars.vars().empty() ) {
			coeff = solver.let(coeff);
		} else {
			coeff = solver.expand(coeff);
		}
	}
	return *this;
}
Poly Poly::eval_coeffs( Smt::Solver& solver ) const {
	Poly ret;
	for( auto& [vars,coeff] : _map ) {
		auto post = coeff.post();
		assert(post);
		ret._map.emplace(vars,solver.get_value(*post));
	}
	return ret;
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

Algebra<Template::Fun,Poly> const Poly::ALGEBRA =
	[]( Template::Fun const& f, std::vector<Poly> const& args )->Poly {
	if( auto const& smt = f.is_smt() ) {
		return *smt;
	}
	if( auto const& sym = f.is_fun() ) {
		if( *sym == Smt::ADD ){
			return sum(args);
		} else if( *sym == Smt::MUL ) {
			return prod(args);
		} else if( *sym == Smt::ITE ) {
			assert( args.size() == 3 );
			return ite(args[0],args[1],args[2]);
		} else {
			assert( args.empty() );
			return Var(*sym,Poly::POS);
		}
	}
	throw Error("#poly:template-algebra");
};

Algebra<Poly::Sig,Poly> Poly::sig_algebra( Smt::Solver& solver ) {
	return [&solver]( Poly::Sig const& f, std::vector<Poly> const& args )->Poly{
		if( f.ref<Add>() ) {
			return sum(args);
		}
		if( f.ref<Mul>() ) {
			return prod(args);
		}
		if( auto const& i = f.ref<Cond>() ) {
			return ite(i->exp,args[0],args[1]);
		}
		assert( args.size() == 0 );
		if( auto const& var = f.ref<Var>() ) {
			return Var(*var,Poly::POS);
		}
		if( auto const& c = f.ref<Smt::PreExp>() ) {
			return *c;
		}
		assert(false);
	};
}

Term<Sum<Poly::Sig,Arg>> Poly::instantiate(
	Smt::Solver& solver,
	Term<Sum<Sig,Arg>> const& org
) {
	auto const& sym = org.fun();
	auto args = vector<Term<Sum<Sig,Arg>>>();
	for( auto const& a : org.args() ) {
		args.push_back(instantiate(solver,a));
	}
	if( auto const& f = sym.ref<Poly::Sig>() ) {
		auto rargs = vector<Term<Sum<Sig,Arg>>>();
		if( auto pre = f->ref<Smt::PreExp>() ) {
			auto post = pre->post();
			assert( post );
			assert( args.size() == 0 );
			return solver.get_value(*post);
		}
		if( f->ref<Poly::Add>() ) {
			Smt::PostExp smtsum = 0;
			for( auto const& v : args ) {
				if( auto g = v.fun().ref<Poly::Sig>() ) {
					if( auto pre = g->ref<Smt::PreExp>() ) {
						auto post = pre->post();
						assert(post);
						smtsum += *post;
						continue;
					}
					if( g->ref<Poly::Add>() ) {// merge arguments
						std::move(v.args().begin(), v.args().end(), std::back_inserter(rargs));
						continue;
					}
				}
				rargs.push_back(v);
			}
			if( rargs.empty() ) return smtsum;
			if( smtsum != 0 ) {
				rargs.push_back(smtsum);
			}
			if( rargs.size() == 1 ) return rargs[0];
			return app(Poly::ADD,std::move(rargs));
		}
		if( f->ref<Poly::Mul>() ) {
			Smt::PostExp smtprod = 1;
			for( auto const& v : args ) {
				if( auto g = v.fun().ref<Poly::Sig>() )
					if( auto pre = g->ref<Smt::PreExp>() ) {
						auto post = pre->post();
						assert(post);
						smtprod.mul_eq(*post,solver.logic().linear());
						continue;
					}
				rargs.push_back(v);
			}
			if( rargs.empty() || smtprod == 0 ) return smtprod;
			if( smtprod != 1 ) {
				rargs.push_back(smtprod);
			}
			if( rargs.size() == 1 ) return rargs[0];
			return app(Poly::MUL,std::move(rargs));
		}
		if( auto const& cond = f->ref<Poly::Cond>() ) {
			assert( args.size() == 2 );
			auto post = cond->exp.post();
			assert(post);
			auto condval = solver.get_value(*post);
			if( auto b = condval.is_bool() ) {
				return args[ *b ? 0 : 1 ];
			}
			return app(Poly::Cond(condval),std::move(args));
		}
	}
	return app(sym,std::move(args));
}

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
			ret._set.emplace_back( xp * yp );
		}
	}
	return std::move(ret);
}

Smt::Compare order( MPoly const& x, MPoly const& y, Smt::Solver& solver ) {
	Smt::PostExp ge_all = true, gt_all = true;
	for( auto const& yp : y._set ){
		Smt::PostExp some_ge, some_gt;
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
	auto hsubst = Deriver<string,Template::Fun>{
		{ "f", Term<Sum<Template::Fun,Arg>>(
			string("+"),
			Term<Sum<Template::Fun,Arg>>("*",c1,Arg(0)),
			Term<Sum<Template::Fun,Arg>>("*",c2,Arg(1))
		)},
		{ "a", Term<Sum<Template::Fun,Arg>>(string("+"),Arg(0),wa) },
		{ "b", Term<Sum<Template::Fun,Arg>>(string("+"),Arg(0),wb) }
	};
	auto e = Exp{"f",Exp{"a","x"},Exp{"b","x"}};
	cout << "⟦" << e << "⟧ = " << hsubst.derive(TERM<Template::Fun>)(e) << endl;
	cout << hsubst.derive(Poly::ALGEBRA)(e) << endl;
	Poly x = Poly::Var("x",Poly::POS);
	Poly y = Poly::Var("y",Poly::NEG);
	auto c = z3.declare_const("c",Smt::INT);
	auto d = z3.declare_const("d",Smt::INT);
	Poly p = x * c + d, q = x * 5 + 3;
	cout << "Poly: " << p << " >= " << q << endl;
	cout << "Smt: " << z3.expand(p.ge(q)) << endl;

	return 0;
}
