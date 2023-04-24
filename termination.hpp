#ifndef TERMINATION_HPP_
#define TERMINATION_HPP_

#include<list>
#include"trs.hpp"
#include"template.hpp"
#include"poly.hpp"

class MonoProc {
	Trs::Rules& rules;
	std::set<size_t>& used;
	std::vector<std::pair<Smt::PostExp,Smt::PostExp>> ords;
	Smt::Solver& solver;
	Deriver::Map deriver;
	Algebra::Intp<Poly> intp;
	MonoProc( MonoProc const& ) = delete;
public:
	MonoProc( Trs::Sig const& sig, Trs::Rules& rules, std::set<size_t>& used, Template const& temp, Smt::Solver& solver );
	std::vector<size_t> remove();
};

MonoProc::MonoProc(
	Trs::Sig const& sig,
	Trs::Rules& rules,
	std::set<size_t>& used,
	Template const& temp,
	Smt::Solver& solver
) : rules(rules), used(used), ords(rules.size()), solver(solver),
	deriver(temp.deriver(sig,solver)),
	intp(deriver.derive(Poly::ALGEBRA)) {
	for( size_t i : used ) {
		auto const& ord = solver.expand(
			Smt::Let((Smt::BOOL,Smt::BOOL)) ^ intp.eval(rules[i].first).order(intp.eval(rules[i].second)) ^ []( Smt::PreExp const& val ){
				return val;
			}
		);
		ords[i] = { Smt::Car(ord), Smt::Cdr(ord) };
	}
}

std::vector<size_t> MonoProc::remove() {
	Smt::PostExp conj = Smt::TRUE;
	Smt::PostExp disj = Smt::FALSE;
	solver.push();
	for( size_t i : used ) {
		conj.conj_eq(ords[i].first);
		disj.disj_eq(ords[i].second);
	}
	solver.ass(conj);
	solver.ass(disj);
	solver.check_sat();
	std::vector<size_t> ret;
	if( solver.result().is_sat() ) {
		for( size_t i : used ) {
			if( solver.get_value(ords[i].second) == Smt::TRUE ) {
				used.erase(i);
				ret.push_back(i);
			}
		}
	}
	solver.pop();
	return ret;
}


class DpProc {

};


#endif