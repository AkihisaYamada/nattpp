#ifndef DERIVER_HPP
#define DERIVER_HPP
#include"smt.hpp"
#include"trs.hpp"

struct Template {
	Template() = delete;
	struct Fun {
	private:
		Sum<std::string,Smt::PreExp> _sum;
	public:
		template<typename... Args>
		Fun( Args&&... args ) : _sum(std::forward<Args>(args)...) {}
		Opt<std::string const&> is_fun() const& {
			return _sum.ref<std::string>();
		}
		Opt<Smt::PreExp const&> is_smt() const& {
			return _sum.ref<Smt::PreExp>();
		}
	};
	static Deriver<std::string,Fun> deriver_of( ::Exp const& e, Trs::Sig const& sig, Smt::Solver& solver );

	/** instantiate SMT expressions via get_value */
	static Algebra<Sum<Fun,Arg>,ArgTerm<Fun>> instantiator( Smt::Solver& solver );

	static ::Exp const SUM, MONO_SUM, MONO_POLY2;

	static void test();
};

ArgTerm<Template::Fun>& operator+=( ArgTerm<Template::Fun>& x, ArgTerm<Template::Fun> const& y );
ArgTerm<Template::Fun>& operator*=( ArgTerm<Template::Fun>& x, ArgTerm<Template::Fun> const& y );
ArgTerm<Template::Fun> ite(
	ArgTerm<Template::Fun> const& i,
	ArgTerm<Template::Fun> const& t,
	ArgTerm<Template::Fun> const& e
);

inline std::ostream& operator<<( std::ostream& os, Template::Fun const& sym ) {
	if( auto const& e = sym.is_smt() ) {
		return os << *e;
	}
	if( auto const& f = sym.is_fun() ) {
		return os << *f;
	}
	assert(false);
}

#endif