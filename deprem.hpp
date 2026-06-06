#ifndef DEPREM_HPP
#define DEPREM_HPP

#include "termord.hpp"
#include "dp.hpp"

class DepRemover {
	TermOrder _term_order;
	Dp::Rules& _rules;
public:
	DepRemover( TermOrder&& org, Dp::Rules& rules ) :
		_term_order(std::move(org)), _rules(rules)
	{
		
	}
};

#endif