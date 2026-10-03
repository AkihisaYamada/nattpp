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
		std::string name;
	};
	struct Sym {
	private:
		using _Sum = Sum<std::string,Fun,Smt::PreExp>;
		_Sum _sum;
	public:
		template<typename... Args>
			requires std::is_constructible_v<_Sum,Args...>
		Sym( Args&&... args ) : _sum(std::forward<Args>(args)...) {}
		Opt<std::string const&> is_var() const& {
			return _sum.ref<std::string>();
		}
		Opt<Fun const&> is_fun() const& {
			return _sum.ref<Fun>();
		}
		Opt<Smt::PreExp const&> is_smt() const& {
			return _sum.ref<Smt::PreExp>();
		}
	};
	struct Deriver : ::Deriver<std::string,Sym> {
	private:
		Deriver() = delete;
		Smt::Solver& _solver;
		Exp _template_exp;
//		std::function<Smt::PostExp(std::string const&, size_t)> const simp;
	public:
		int log;
		Smt::PostExp const mono;
		struct ArgInfo {
			Smt::PostExp infl, used;
		};
		struct FunInfo {
			Smt::PostExp triv;
			std::vector<ArgInfo> args;
		};
		Map<std::string,FunInfo> sig;
		Deriver(
			Exp const& e,
			Smt::Solver& solver,
			Smt::PostExp const& mono,
			int log
		) : _template_exp(e),
			_solver(solver),
			mono(mono),
			log(log) {}
		void extend_sig( std::string const& f, Trs::Rank const& rank ) &;
		void extend_sig( Trs::Sig const& sig ) & {
			for( auto [f,rank] : sig ) {
				extend_sig(f,rank);
			}
		}
	private:
		Term<Sum<Sym,Arg>> _deriver_of(
			Exp const& exp,
			std::string const& f,
			Trs::Rank const& rank,
			int pos,
			FunInfo const& finfo
		);
	};
	/** instantiate SMT expressions in templates via get_value */
	static Algebra<Sum<Sym,Arg>,ArgTerm<Sym>> instantiator( Smt::Solver& solver );

	static ::Exp const SUM, MONO_SUM, MONO_POLY2, SIMP_MAX, MAX, IMAX, MAT2B, MAT2N, POSNEG;

	static void test();
};

ArgTerm<Template::Sym>& operator+=( ArgTerm<Template::Sym>& x, ArgTerm<Template::Sym> const& y );
ArgTerm<Template::Sym>& operator*=( ArgTerm<Template::Sym>& x, ArgTerm<Template::Sym> const& y );
ArgTerm<Template::Sym> ite(
	ArgTerm<Template::Sym> const& i,
	ArgTerm<Template::Sym> const& t,
	ArgTerm<Template::Sym> const& e
);
ArgTerm<Template::Sym>& max_eq( ArgTerm<Template::Sym>& x, ArgTerm<Template::Sym> const& y );

inline std::ostream& operator<<( std::ostream& os, Template::Sym const& sym ) {
	if( auto const& e = sym.is_smt() ) {
		return os << *e;
	}
	if( auto const& f = sym.is_fun() ) {
		return os << f->name;
	}
	if( auto const& f = sym.is_var() ) {
		return os << *f;
	}
	assert(false);
}

#endif