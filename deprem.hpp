#ifndef DEPREM_HPP
#define DEPREM_HPP

#include "termord.hpp"
#include "dp.hpp"

class DepRemover {
	TrsPosOrder _term_order;
	Map<size_t,Dp>& _rules;
public:
	DepRemover( TrsPosOrder&& org, Map<size_t,Dp>& rules ) :
		_term_order(std::move(org)), _rules(rules)
	{
		
	}
};

#endif