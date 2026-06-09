#ifndef TERMORD_HPP_
#define TERMORD_HPP_

#include<list>
#include"smt.hpp"

struct TermOrder {
	virtual Smt::Solver& solver() = 0;
	virtual std::ostream& print_name( std::ostream& os ) = 0;
	virtual std::ostream& print( std::ostream& os, Trs::Sig const& sig ) = 0;
	virtual Smt::Compare compare( Exp const& l, Exp const& r ) = 0;
};

struct TrsOrder : TermOrder {
	virtual Smt::Compare order_rule( size_t i ) & = 0;
	static std::unique_ptr<TrsOrder> of(
		Exp const& x,
		Trs::Sig const& sig,
		Trs::Rules const& trs,
		bool mono,
		std::function<Smt::Solver()> const& default_smt,
		Smt::Sort const& default_sort
	);
};

struct TrsPosOrder : TrsOrder {
	virtual Smt::Compare order_rule( size_t i, Pos const& l, Pos const& r ) & = 0;
	Smt::Compare order_rule( size_t i ) & override {
		return order_rule(i,{},{});
	}
};

struct TrivOrder : TrsOrder {
	Smt::Solver _solver;
	TrivOrder( Smt::Solver&& sol ) : _solver(std::move(sol)) {}
	Smt::Solver& solver() override {
		return _solver;
	}
	Smt::Compare compare( Exp const& l, Exp const& r ) override {
		return {Smt::TRUE,Smt::FALSE};
	};
	std::ostream& print_name( std::ostream& os ) override {
		return os << "trivial-order";
	};
	std::ostream& print( std::ostream& os, Trs::Sig const& sig ) override {
		return os;
	}
	Smt::Compare order_rule( size_t i ) & override {
		return {Smt::TRUE,Smt::FALSE};
	}
};

template<typename A>
struct DerivedTermOrder : TermOrder {
	Smt::Solver _solver;
	Algebra::Intp<std::string,A> const intp;
	Algebra::Deriver<std::string, typename A::Sig> const deriver;
	DerivedTermOrder(
		Trs::Sig const& sig,
		A::Template const& temp,
		Smt::Solver&& sol_,
		Smt::Sort const& sort
	) : _solver(std::move(sol_)),
		deriver(temp.deriver(sig,_solver)),
		intp(A::expand(deriver.derive(A::algebra(_solver)),_solver,sort)) {
	}
	Smt::Compare compare( Exp const& l, Exp const& r ) override {
		return A::compare(intp.eval(l),intp.eval(r),_solver);
	}
	std::ostream& print_name( std::ostream& os ) override {
		return os << "derived-order";
	};
	std::ostream& print( std::ostream& os, Trs::Sig const& sig ) override {
		os << '(';
		for( auto [f,arity] : sig ) {
			os << "\n    (" << f << ' ' << A::instantiate(solver(),deriver(f)) << ')';
		}
		return os << ')';
	}
	Smt::Solver& solver() override {
		return _solver;
	}
};

std::vector<size_t> order_some_rule( TrsOrder& order, Trs::Rules const& rules );

template<typename A>
struct DerivedTrsOrder : TrsOrder {
private:
	DerivedTermOrder<A> _term_order;
	Map<size_t,Smt::Compare> _ords;
public:
	DerivedTrsOrder(
		Trs::Sig const& sig,
		Trs::Rules& rules,
		A::Template const& temp,
		Smt::Solver&& sol_,
		Smt::Sort const& sort
	) : _term_order(sig,temp,std::move(sol_),sort) {
		auto& sol = solver();
		for( auto const& [i,rule] : rules ) {
			auto [ge,gt] = _term_order.compare(rule.first,rule.second);
			auto gev = sol.let(Smt::BOOL,ge);
			auto gtv = sol.let(Smt::BOOL,gt);
			_ords.insert(i,Smt::Compare{gev,gtv});
		}
	}
	Smt::Solver& solver() override { return _term_order.solver(); }
	Smt::Compare order_rule( size_t i ) & override {
		auto o = _ords.find(i);
		assert(o);
		return *o;
	}
	Smt::Compare compare( Exp const& l, Exp const& r ) override {
		return _term_order.compare(l,r);
	}
	std::ostream& print_name( std::ostream& os ) override {
		return _term_order.print_name(os);
	}
	std::ostream& print( std::ostream& os, Trs::Sig const& sig ) override {
		return _term_order.print(os,sig);
	}
};

template<typename A>
struct DerivedTrsPosOrder : TrsPosOrder {
	using ASig = std::pair<std::string,A>;
protected:
	DerivedTermOrder<A> _term_order;
	Map<size_t,std::pair<Term<ASig>,Term<ASig>>> _arules;
public:
	DerivedTrsPosOrder(
		Trs::Sig const& sig,
		Trs::Rules const& rules,
		A::Template const& temp,
		Smt::Solver&& solver,
		Smt::Sort const& sort
	) : _term_order(sig,temp,std::move(solver),sort) {
		for( auto [n,rule] : rules ) {
			_arules.insert(n,std::pair{_term_order.intp.annotate(rule.first),_term_order.intp.annotate(rule.second)});
		}
	}
	Smt::Solver& solver() override { return _term_order.solver(); }
	Smt::Compare order_rule( size_t i, Pos const& lpos, Pos const& rpos ) & override {
		auto arule = _arules.find(i);
		assert(arule);
		auto lv = arule->first.at(lpos).fun().second;
		auto rv = arule->second.at(rpos).fun().second;
		return A::compare(lv,rv,solver());
	}
	Smt::Compare compare( Exp const& l, Exp const& r ) override {
		return _term_order.compare(l,r);
	}
	std::ostream& print_name( std::ostream& os ) override {
		return _term_order.print_name(os);
	}
	std::ostream& print( std::ostream& os, Trs::Sig const& sig ) override {
		return _term_order.print(os,sig);
	}
};

struct PathOrder : TrsOrder {
	struct SigInfo {
		Smt::PostExp prec;
	};
private:
	std::unique_ptr<TermOrder> _weight;
	Map<std::string,SigInfo> _sig;
	Map<size_t,Smt::Compare> _ord;
	Map<std::pair<Term<std::string>,Term<std::string>>,Smt::Compare> _table;
public:
	PathOrder(
		Trs::Sig const& sig,
		Trs::Rules const& rules,
		std::unique_ptr<TermOrder>&& weight,
		Smt::BaseSort const& prec_sort
	) : _weight(std::move(weight)) {
		size_t sigsize = sig.size();
		for( auto const&[fun1,rank1] : sig ) {
			auto const& prec = solver().declare_fresh(prec_sort);
			_sig.insert(fun1,prec);
		}
		for( auto const& [n,rule] : rules ) {
			auto const& l = rule.first;
			auto const& r = rule.second;
			_ord.insert(n,compare(l,r));
		}
	}
	std::ostream& print_name( std::ostream& os ) override {
		return _weight->print_name( os << "(path-order " );
	}
	std::ostream& print( std::ostream& os, Trs::Sig const& sig ) override {
		return os << "blahblah";
	}
	Smt::Solver& solver() override {
		return _weight->solver();
	}
	Smt::Compare compare( Exp const& l, Exp const& r ) override;
	Smt::Compare order_rule( size_t i ) & override {
		auto opt = _ord.find(i);
		assert( opt );
		return *opt;
	}
};


#endif
