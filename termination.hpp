#ifndef TERMINATION_HPP_
#define TERMINATION_HPP_

#include<list>
#include"trs.hpp"
#include"poly.hpp"

class TrsAnnotator {
	
};

struct TermOrder {
	virtual Smt::PreExp operator()( Exp const& l, Exp const& r ) = 0;
	virtual std::ostream& print_name( std::ostream& os ) = 0;
	virtual std::ostream& print( std::ostream& os, Trs::Sig const& sig ) = 0;
};

struct TrivOrder : TermOrder {
	Smt::PreExp operator()( Exp const& l, Exp const& r ) override {
		return (Smt::TRUE,Smt::FALSE);
	};
	std::ostream& print_name( std::ostream& os ) override {
		return os << "trivial-order";
	};
	std::ostream& print( std::ostream& os, Trs::Sig const& sig ) override {
		return os;
	}
};

template<typename A>
class DerivedTermOrder : public TermOrder {
	Smt::Solver& solver;
	DerivedTermOrder( DerivedTermOrder const& ) = delete;
public:
	DerivedTermOrder( DerivedTermOrder && ) = default;
	Algebra::Intp<std::string,A> const intp;
	Algebra::Deriver<std::string, typename A::Sig> const deriver;
	DerivedTermOrder(
		Trs::Sig const& sig,
		A::Template const& temp,
		Smt::Solver& solver,
		Smt::BaseSort const& sort
	) : solver(solver),
		deriver(temp.deriver(sig,solver)),
		intp(A::expand(deriver.derive(A::algebra(solver)),solver,sort)) {
	}
	Smt::PreExp operator()( Exp const& l, Exp const& r ) override {
		return A::order(intp.eval(l),intp.eval(r),solver);
	}
	std::ostream& print_name( std::ostream& os ) override {
		return os << "derived-order";
	};
	std::ostream& print( std::ostream& os, Trs::Sig const& sig ) override {
		os << '(';
		for( auto [f,arity] : sig ) {
			os << "\n    (" << f << ' ' << A::instantiate(solver,deriver(f)) << ')';
		}
		return os << ')';
	}
};

class PathOrder : public TermOrder {
	struct SigInfo {
		Smt::PostExp prec;
	};
	Map<std::string,SigInfo> _map;
public:
	TermOrder& weight;
	PathOrder( Trs::Sig const& sig, TermOrder& weight, Smt::Solver& solver ) : weight(weight) {
		for( auto it1 = sig.begin(); it1 != sig.end(); ) {
			auto const& [fun1,rank1] = *it1;
			for( auto it2 = sig.begin(); it2 != it1; it2++ ) {
				_map.insert(fun1,SigInfo{solver.declare_fresh(Smt::INT)});
				
			}
		}
	}
	Smt::PreExp operator()( Exp const& l, Exp const& r ) override {
		throw Error("#unsupported");
	}
};

class RuleRemover : public TermOrder {
	std::unique_ptr<TermOrder> _ptr;
protected:
	Trs::Rules& rules;
	std::set<size_t>& used;
	std::vector<std::pair<Smt::PostExp,Smt::PostExp>> ords;
	Smt::Solver& solver;
public:
	RuleRemover(
		std::unique_ptr<TermOrder>&& ptr,
		Trs::Rules& rules,
		std::set<size_t>& used,
		Smt::Solver& solver
	);
	std::vector<size_t> remove();
	Smt::PreExp operator()( Exp const& l, Exp const& r ) override {
		return (*_ptr)(l,r);
	}
	std::ostream& print_name( std::ostream& os ) override {
		return _ptr->print_name(os);
	}
	std::ostream& print( std::ostream& os, Trs::Sig const& sig ) override {
		return _ptr->print(os,sig);
	}
};

#endif