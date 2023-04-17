#include"algebra.hpp"

using namespace std;

const Algebra::Intp<Exp> Algebra::TERM = {
	[](std::string_view const& f, std::vector<Exp>&& args ){
		return Exp(f,std::move(args));
	}
};

ostream& operator<<( ostream& os, Subst const& subst ) {
	os << '[' << endl;
	for( auto const& [key,val] : subst ) {
		os << '\t' << key << " := " << val << endl;
	}
	return os << ']';
}
