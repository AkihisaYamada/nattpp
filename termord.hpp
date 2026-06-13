#ifndef TERMORD_HPP_
#define TERMORD_HPP_

#include"trs.hpp"
#include"smt.hpp"

struct TermOrder {
    virtual ~TermOrder() = default;// to be able to make pointer of TermOrder 
	enum { NONE = 0, RULE = 1 << 1, PAIR = 1 << 2, LOG = 1 << 3 };
	virtual int verbosity() { return NONE; };
	virtual Smt::Solver& solver() = 0;
	virtual std::ostream& print_name( std::ostream& os ) = 0;
	Printable print_name() & {
		return Printable([&]( std::ostream& os )->std::ostream&{
			return print_name(os);
		});
	}
	virtual std::ostream& print_sym_info( std::ostream& os, std::string const& f ) = 0;
	Printable print_sym_info( std::string const& f ) {
		return Printable([&]( std::ostream& os )->std::ostream&{
			return print_sym_info(os,f);
		});
	}
	virtual std::ostream& print( std::ostream& os, Trs::Sig const& sig ) {
		os << '(' << print_name();
		for( auto [f,arity] : sig ) {
			os << "\n    (" << f << ' ' << print_sym_info(f) << ')';
		}
		return os << ')';
	}
	Printable print( Trs::Sig const& sig ) & {
		return Printable([&]( std::ostream& os )->std::ostream&{
			return print(os,sig);
		});
	}
	virtual Smt::Compare compare( Exp const& l, Exp const& r ) = 0;
	static void test();
};

struct TrsOrder : TermOrder {
	virtual Smt::Compare order_rule( size_t i ) & = 0;
	static std::unique_ptr<TrsOrder> of(
		Exp const& x,
		Trs::Sig const& sig,
		Trs::Rules const& trs,
		bool mono,
		std::function<Smt::Solver()> const& default_smt,
		Smt::Sort const& default_sort
	);
};

struct TrsPosOrder : TrsOrder {
	virtual Smt::Compare order_rule( size_t i, Pos const& l, Pos const& r ) & = 0;
	Smt::Compare order_rule( size_t i ) & override {
		return order_rule(i,{},{});
	}
};

struct TrivOrder : TrsOrder {
	Smt::Solver _solver;
	TrivOrder( Smt::Solver&& sol ) : _solver(std::move(sol)) {}
	Smt::Solver& solver() override {
		return _solver;
	}
	Smt::Compare compare( Exp const& l, Exp const& r ) override {
		return {Smt::TRUE,Smt::FALSE};
	};
	std::ostream& print_name( std::ostream& os ) override {
		return os << "trivial-order";
	};
	std::ostream& print_sym_info( std::ostream& os, std::string const& sig ) override {
		return os;
	}
	Smt::Compare order_rule( size_t i ) & override {
		return {Smt::TRUE,Smt::FALSE};
	}
};

template<typename A>
struct DerivedTermOrder : TermOrder {
private:
	Smt::Solver _solver;
	int _verbosity;
public:
	Algebra::Deriver<std::string, typename A::Sig> const deriver;
	Algebra::Intp<std::string,A> const intp;
	DerivedTermOrder(
		Trs::Sig const& sig,
		A::Template const& temp,
		Smt::Solver&& sol_,
		int verb = NONE
	) : _solver(std::move(sol_)),
		_verbosity(verb),
		deriver(temp.deriver(sig,_solver)),
		intp(A::expand(deriver.derive(A::algebra(_solver)),_solver)) {
	}
	Smt::Compare compare( Exp const& l, Exp const& r ) override {
		return A::compare(intp.eval(l),intp.eval(r),_solver);
	}
	std::ostream& print_name( std::ostream& os ) override {
		return os << "derived-order";
	};
	Smt::Solver& solver() override {
		return _solver;
	}
	std::ostream& print_sym_info( std::ostream& os, std::string const& f ) override {
		return os << ":intp " << A::instantiate(solver(),deriver(f));
	}
	int verbosity() override {
		return _verbosity;
	}
};

std::vector<size_t> order_some_rule( TrsOrder& order, Trs::Rules const& rules );

template<typename A>
struct DerivedTrsOrder : TrsOrder {
private:
	DerivedTermOrder<A> _term_order;
	Map<size_t,Smt::Compare> _ords;
public:
	DerivedTrsOrder(
		Trs::Sig const& sig,
		Trs::Rules& rules,
		A::Template const& temp,
		Smt::Solver&& sol_,
		Smt::Sort const& sort,
		int const& verb = NONE
	) : _term_order(sig,temp,std::move(sol_),sort,verb) {
		auto& sol = solver();
		for( auto const& [i,rule] : rules ) {
			auto [ge,gt] = _term_order.compare(rule.first,rule.second);
			auto gev = sol.let(Smt::BOOL,ge);
			auto gtv = sol.let(Smt::BOOL,gt);
			_ords.emplace(i,Smt::Compare{gev,gtv});
		}
	}
	Smt::Solver& solver() override { return _term_order.solver(); }
	Smt::Compare order_rule( size_t i ) & override {
		auto o = _ords.find(i);
		assert(o);
		return *o;
	}
	Smt::Compare compare( Exp const& l, Exp const& r ) override {
		return _term_order.compare(l,r);
	}
	std::ostream& print_name( std::ostream& os ) override {
		return _term_order.print_name(os);
	}
	std::ostream& print_sym_info( std::ostream& os, std::string const& f ) override {
		return _term_order.print_sym_info(os,f);
	}
	int verbosity() override { return _term_order.verbosity(); }
};

template<typename A>
struct DerivedTrsPosOrder : TrsPosOrder {
	using ASig = std::pair<std::string,A>;
protected:
	DerivedTermOrder<A> _term_order;
	Map<size_t,std::pair<Term<ASig>,Term<ASig>>> _arules;
public:
	DerivedTrsPosOrder(
		Trs::Sig const& sig,
		Trs::Rules const& rules,
		A::Template const& temp,
		Smt::Solver&& solver,
		int const& verb = NONE
	) : _term_order(sig,temp,std::move(solver),verb) {
		for( auto const& [n,rule] : rules ) {
			_arules.emplace(n,std::pair{_term_order.intp.annotate(rule.first),_term_order.intp.annotate(rule.second)});
		}
	}
	Smt::Solver& solver() override { return _term_order.solver(); }
	Smt::Compare order_rule( size_t i, Pos const& lpos, Pos const& rpos ) & override {
		auto arule = _arules.find(i);
		assert(arule);
		auto lv = arule->first.at(lpos).fun().second;
		auto rv = arule->second.at(rpos).fun().second;
		return A::compare(lv,rv,solver());
	}
	Smt::Compare compare( Exp const& l, Exp const& r ) override {
		return _term_order.compare(l,r);
	}
	std::ostream& print_name( std::ostream& os ) override {
		return _term_order.print_name(os);
	}
	std::ostream& print_sym_info( std::ostream& os, std::string const& f ) override {
		return _term_order.print_sym_info(os,f);
	}
	int verbosity() override { return _term_order.verbosity(); }
};

struct PathOrder : TrsOrder {
private:
	struct _SymInfo {
		Smt::PostExp prec;
		int post_arity;// arity after argument rearrangement
		std::vector<std::vector<Smt::PostExp>> map;// map[i][j] i-th argument is mapped to j-th position
		std::vector<Smt::PostExp> mapped;// flags if the corresponding argument is mapped
	};
	std::unique_ptr<TermOrder> _weight;
	Map<std::string,_SymInfo> _info;
	Map<size_t,Smt::Compare> _ord;
	Map<std::pair<Term<std::string>,Term<std::string>>,Smt::Compare> _table;
	int _verbosity;
public:
	struct Status {
		struct Straight {};
		struct Mapped {
			size_t post_arity;
		};
	private:
		Sum<Straight,Mapped> _sum;
	public:
		Status( Straight const& ) : _sum(Straight()) {}
		Status( Mapped b ) : _sum(b) {}
		bool is_straight() { return _sum.ref<Straight>(); }
		Opt<size_t> post_arity() {
			return _sum.ref<Mapped>() >>= [&]( auto b )->Opt<size_t>{ return {b.post_arity}; };
		}
		static std::function<Status(Trs::Rank const&)> of( Exp const&, bool mono );
	};
	PathOrder(
		Trs::Sig const& sig,
		Trs::Rules const& rules,
		std::unique_ptr<TermOrder>&& weight,
		std::function<Status(Trs::Rank const&)> status,
		int verb
	);
	std::ostream& print_name( std::ostream& os ) override {
		return os << "path-order " << _weight->print_name();
	}
	std::ostream& print_sym_info( std::ostream& os, std::string const& f ) override;
	Smt::Solver& solver() override {
		return _weight->solver();
	}
	Smt::Compare compare( Exp const& l, Exp const& r ) override;
	Smt::Compare order_rule( size_t i ) & override {
		auto opt = _ord.find(i);
		assert( opt );
		return *opt;
	}
	int verbosity() override { return _verbosity; }
};

extern Exp const SUM_SPEC, MONO_LPO_SPEC, LPO3_SPEC;

template<typename F, typename T>
Smt::Compare lex_compare( F const& comp, std::vector<T> const& ls, std::vector<T> const& rs ) {
	auto all_ge = Smt::TRUE, gt = Smt::FALSE;
	auto ln = ls.size();
	auto rn = rs.size();
	for( size_t i = 0;; i++ ) {
		if( i == ln ) {
			if( i == rn ) {
				return { gt || all_ge, gt };
			} else {
				return { gt, gt };
			}
		} else if( i == rn ) {
			return { gt || all_ge, gt || all_ge };
		}
		auto const& c = comp(ls[i],rs[i]);
		gt = gt || (all_ge && c.gt);
		all_ge = all_ge && c.ge;
	}
}

template<typename F, typename T>
Smt::Compare mapped_lex_compare(
	F const& comp,
	size_t lpar,// post arity
	size_t rpar,
	std::vector<std::vector<Smt::PostExp>> const& lmap,
	std::vector<std::vector<Smt::PostExp>> const& rmap,
	std::vector<T> const& ls,
	std::vector<T> const& rs
) {
	auto all_ge = Smt::TRUE, gt = Smt::FALSE;
	auto lin = ls.size();
	auto rin = rs.size();
	for( size_t k = 0;; k++ ) {
		if( k == lpar ) {
			if( k == rpar ) {
				return { gt || all_ge, gt };
			} else {
				return { gt, gt };
			}
		} else if( k == rpar ) {
			return { gt || all_ge, gt || all_ge };
		}
		auto ige = Smt::disj( 0, lin, [&]( size_t const& i ){
			return lmap[i][k] && Smt::disj( 0, rin, [&]( size_t const& j ){
				return rmap[j][k] && comp(ls[i],rs[j]).ge;
			} );
		} );
		auto igt = Smt::disj( 0, lin, [&]( size_t const& i ){
			return lmap[i][k] && Smt::disj( 0, rin, [&]( size_t const& j ){
				return rmap[j][k] && comp(ls[i],rs[j]).gt;
			} );
		} );
		gt = gt || (all_ge && igt);
		all_ge = all_ge && ige;
	}
}

#endif
