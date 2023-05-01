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

class DpProc {

};


#endif