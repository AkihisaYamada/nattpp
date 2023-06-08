#ifndef TEMPLATE_HPP_
#define TEMPLATE_HPP_

#include "smt.hpp"

class Template : public Exp {
public:
	struct Error : ::Error {
		using ::Error::Error;
	};
	class Add {};
	static Add constexpr ADD = {};
	class Mul {};
	static Mul constexpr MUL = {};
	using Sig = Sum<Smt::PostExp,Add,Mul>;
	static Algebra::Intp<Sig,Smt::PostExp> const ALGEBRA;
	Template( Exp const& exp ) : Exp(exp) {}
	Algebra::Deriver<std::string,Sig> deriver( Trs::Sig const& sig, Smt::Solver& solver ) const;
	static Template const SUM;
	static Template const MONO_SUM;
private:
	static Tree<Sum<Template::Sig,Algebra::Arg>> _deriver_inner( std::string const& f, Trs::Rank const& rank, Smt::Solver& solver, Exp const& exp, int pos );
};

#endif