#ifndef DERIVER_HPP
#define DERIVER_HPP
#include"smt.hpp"
#include"trs.hpp"

/** SMT extended with custom functions.
 * @todo Consider moving features from smt.hpp
 */
struct Template {
	Template() = delete;
	static std::string const MONO;
	struct Fun {
	private:
		using _Sum = Sum<std::string,Smt::PreExp>;
		_Sum _sum;
	public:
		template<typename... Args>
			requires std::is_constructible_v<_Sum,Args...>
		Fun( Args&&... args ) : _sum(std::forward<Args>(args)...) {}
		Opt<std::string const&> is_fun() const& {
			return _sum.ref<std::string>();
		}
		Opt<Smt::PreExp const&> is_smt() const& {
			return _sum.ref<Smt::PreExp>();
		}
	};
	struct Deriver : ::Deriver<std::string,Fun> {
		Deriver() = delete;
		Smt::PostExp const mono;
//		std::function<Smt::PostExp(std::string const&, size_t)> const simp;
		static Deriver of( Exp const& e, Trs::Sig const& sig, Smt::Solver& solver );
	private:
		Deriver(
			::Deriver<std::string,Fun>&& der,
			Smt::PostExp&& mono
//			std::function<Smt::PostExp(std::string const&, size_t)>&& simp
		) : ::Deriver<std::string,Fun>(std::move(der)), mono(std::move(mono)) {}
	};
	/** instantiate SMT expressions in templates via get_value */
	static Algebra<Sum<Fun,Arg>,ArgTerm<Fun>> instantiator( Smt::Solver& solver );

	static ::Exp const SUM, MONO_SUM, MONO_POLY2, SIMP_MAX;

	static void test();
};

ArgTerm<Template::Fun>& operator+=( ArgTerm<Template::Fun>& x, ArgTerm<Template::Fun> const& y );
ArgTerm<Template::Fun>& operator*=( ArgTerm<Template::Fun>& x, ArgTerm<Template::Fun> const& y );
ArgTerm<Template::Fun> ite(
	ArgTerm<Template::Fun> const& i,
	ArgTerm<Template::Fun> const& t,
	ArgTerm<Template::Fun> const& e
);
ArgTerm<Template::Fun>& max_eq( ArgTerm<Template::Fun>& x, ArgTerm<Template::Fun> const& y );

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