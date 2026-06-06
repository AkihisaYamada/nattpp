#ifndef TERMORD_HPP_
#define TERMORD_HPP_

#include<list>
#include"smt.hpp"

struct Printable {
	virtual std::ostream& print_name( std::ostream& os ) = 0;
	virtual std::ostream& print( std::ostream& os, Trs::Sig const& sig ) = 0;
};
struct TermOrderInterface : Printable {
	virtual Smt::PreExp operator()( Exp const& l, Exp const& r ) = 0;
};
struct TrivOrder : TermOrderInterface {
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

class TermOrder : public TermOrderInterface {
	std::unique_ptr<TermOrderInterface> _ptr;
public:
	template<typename T> requires std::is_base_of_v<TermOrderInterface,T>
	TermOrder( T&& orig ) : _ptr(std::make_unique<T>(std::move(orig))) {}
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

struct TrsOrderInterface : Printable {
protected:
	virtual Trs::Rules const& rules() = 0;
	virtual Smt::Solver& solver() = 0;
public:
	virtual std::pair<Smt::PostExp,Smt::PostExp> order_rule( size_t i ) const& = 0;
	std::vector<size_t> order_some() &;
	friend class TrsOrder;
};

struct TrsOrder : TrsOrderInterface {
private:
	std::unique_ptr<TrsOrderInterface> _ptr;
protected:
	Trs::Rules const& rules() override { return _ptr->rules(); };
	Smt::Solver& solver() override {return _ptr->solver(); };
public:
	template<typename T> requires std::is_base_of_v<TrsOrderInterface,T>
	TrsOrder( T&& orig ) : _ptr(std::make_unique<T>(std::move(orig))) {}
	std::pair<Smt::PostExp,Smt::PostExp> order_rule( size_t i ) const& {
		return _ptr->order_rule(i);
	}
	std::ostream& print_name( std::ostream& os ) override {
		return _ptr->print_name(os);
	}
	std::ostream& print( std::ostream& os, Trs::Sig const& sig ) override {
		return _ptr->print(os,sig);
	}
};

struct TrsPosOrderInterface : TrsOrderInterface {
	virtual std::pair<Smt::PostExp,Smt::PostExp> order_rule( size_t i, Pos const& l, Pos const& r ) const& = 0;
};

template<typename A>
class DerivedTermOrder : public TermOrderInterface {
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

class PathOrder : public TermOrderInterface {
	struct SigInfo {
		Smt::PostExp prec;
	};
	Map<std::string,SigInfo> _map;
public:
	TermOrderInterface& weight;
	PathOrder( Trs::Sig const& sig, TermOrderInterface& weight, Smt::Solver& solver ) : weight(weight) {
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

struct TrsOrderOfTermOrder : TrsOrderInterface {
	Map<size_t,std::pair<Smt::PostExp,Smt::PostExp>> _ords;
	Trs::Rules& _rules;
	Smt::Solver& _solver;
	TermOrder _term_order;
protected:
	Trs::Rules const& rules() { return _rules; };
	Smt::Solver& solver() { return _solver; };
public:
	TrsOrderOfTermOrder(
		TermOrder&& org,
		Trs::Rules& rules,
		Smt::Solver& solver
	) : _term_order(std::move(org)), _rules(rules), _solver(solver) {
		for( auto const& [i,rule] : rules ) {
			auto const& ord = solver.expand(
				Smt::Let( (Smt::BOOL,Smt::BOOL), _term_order(rule.first,rule.second)) ^
				[]( Smt::PreExp const& val ){ return val; }
			);
			_ords.insert(i,std::pair(Smt::car(ord),Smt::cdr(ord)));
		}
	}
	std::pair<Smt::PostExp,Smt::PostExp> order_rule( size_t i ) const& {
		auto o = _ords.find(i);
		assert(o);
		return *o;
	}
	std::ostream& print_name( std::ostream& os ) override {
		return _term_order.print_name(os);
	}
	std::ostream& print( std::ostream& os, Trs::Sig const& sig ) override {
		return _term_order.print(os,sig);
	}
};

#endif
