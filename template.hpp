#ifndef DERIVER_HPP
#define DERIVER_HPP
#include"smt.hpp"
#include"trs.hpp"

/** SMT extended with custom functions.
 * @todo Consider moving features from smt.hpp
 */
struct Template {
	Template() = delete;
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
	private:
		Deriver() = delete;
		Smt::Solver& _solver;
		Exp _template_exp;
//		std::function<Smt::PostExp(std::string const&, size_t)> const simp;
	public:
		Smt::PostExp const mono;
		struct ArgInfo {
			Smt::PostExp inflationary, constant;
		};
		Map<std::string,std::vector<ArgInfo>> sig;
		Deriver(
			Exp const& e,
			Smt::Solver& solver
//			std::function<Smt::PostExp(std::string const&, size_t)>&& simp
		) : _template_exp(e),
			_solver(solver),
			mono(solver.declare_fresh(Smt::BOOL)) {}
		void extend_sig( std::string const& f, Trs::Rank const& rank ) &;
		void extend_sig( Trs::Sig const& sig ) & {
			for( auto [f,rank] : sig ) {
				extend_sig(f,rank);
			}
		}
	private:
		Term<Sum<Fun,Arg>> _deriver_of(
			Exp const& exp,
			std::string const& f,
			Trs::Rank const& rank,
			int pos,
			std::vector<ArgInfo> const& sig
		);
	};
	/** instantiate SMT expressions in templates via get_value */
	static Algebra<Sum<Fun,Arg>,ArgTerm<Fun>> instantiator( Smt::Solver& solver );

	static ::Exp const SUM, MONO_SUM, MONO_POLY2, SIMP_MAX, MAX;

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