#ifndef TEMPLATE_HPP_
#define TEMPLATE_HPP_

#include "smt.hpp"

class Template : public Exp {
public:
	struct Error : ::Error {
		using ::Error::Error;
	};
	Template( Exp const& exp ) : Exp(exp) {}
	Deriver::Map deriver( Trs::Sig const& sig, Smt::Solver& solver ) const;
	static Template const SUM;
	static Template const MONO_SUM;
};

#endif