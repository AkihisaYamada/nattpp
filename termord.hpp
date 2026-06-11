#ifndef TERMORD_HPP_
#define TERMORD_HPP_

#include"trs.hpp"
#include"smt.hpp"

struct TermOrder {
    virtual ~TermOrder() = default;// to be able to make pointer of TermOrder 
	enum { NONE = 0, RULE = 1 << 1, PAIR = 1 << 2 };
	virtual int verbosity() { return NONE; };
	virtual Smt::Solver& solver() = 0;
	virtual std::ostream& print_name( std::ostream& os ) = 0;
	virtual std::ostream& print_sym_info( std::ostream& os, std::string const& f ) = 0;
	virtual std::ostream& print( std::ostream& os, Trs::Sig const& sig ) {
		print_name( os << '(' );
		for( auto [f,arity] : sig ) {
			print_sym_info( os << "\n    (" << f << ' ', f ) << ')';
		}
		return os << ')';
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
			_ords.insert(i,Smt::Compare{gev,gtv});
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
		for( auto [n,rule] : rules ) {
			_arules.insert(n,std::pair{_term_order.intp.annotate(rule.first),_term_order.intp.annotate(rule.second)});
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
		std::vector<std::vector<Smt::PostExp>> map;// map[i][j] means i-th position is taken by j-th argument
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
		static std::function<Status(Trs::Rank const&)> of( Exp const& );
	};
	PathOrder(
		Trs::Sig const& sig,
		Trs::Rules const& rules,
		std::unique_ptr<TermOrder>&& weight,
		std::function<Status(Trs::Rank const&)> status,
		int verb
	) : _weight(std::move(weight)), _verbosity(verb) {
		size_t sigsize = sig.size();
		auto& sol = solver();
		auto const& sort = sol.logic().base_sort();
		for( auto const&[f,rank] : sig ) {
			_SymInfo info;
			info.prec = sol.declare_fresh(sort);
			if( auto post_arity = status(rank).post_arity() ) {
				for( size_t i = 0; i < *post_arity; i++ ) {// i-th position after rearrangement
					auto postmap = std::vector<Smt::PostExp>();
					for( size_t j = 0; j < rank.arity; j++ ) {
						postmap.push_back( sol.declare_fresh(Smt::BOOL) );
						for( size_t k = 0; k < j; k++ ) {// i-th position cannot be shared
							sol.ass( !postmap[k] || !postmap[j] );
						}
					}
				}
			}
			_info.insert(f,info);
		}
		for( auto const& [n,rule] : rules ) {
			auto const& l = rule.first;
			auto const& r = rule.second;
			_ord.insert(n,compare(l,r));
		}
	}
	std::ostream& print_name( std::ostream& os ) override {
		return _weight->print_name( os << "path-order " );
	}
	std::ostream& print_sym_info( std::ostream& os, std::string const& f ) override {
		auto info = _info.find(f);
		assert(info);
		return _weight->print_sym_info( os << ":prec " << solver().get_value(info->prec), f );
	}
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
	std::vector<std::vector<Smt::PostExp>> const& lmap,
	std::vector<std::vector<Smt::PostExp>> const& rmap,
	std::vector<T> const& ls,
	std::vector<T> const& rs
) {
	auto all_ge = Smt::TRUE, gt = Smt::FALSE;
	auto ln = lmap.size();
	auto rn = rmap.size();
	auto lin = ls.size();
	auto rin = rs.size();
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
		auto ige = Smt::disj( 0, lin, [&]( size_t const& j ){
			return lmap[i][j] && Smt::disj( 0, rin, [&]( size_t const& k ){
				return rmap[i][k] && comp(ls[j],rs[k]).ge;
			} );
		} );
		auto igt = Smt::disj( 0, lin, [&]( size_t const& j ){
			return lmap[i][j] && Smt::disj( 0, rin, [&]( size_t const& k ){
				return rmap[i][k] && comp(ls[j],rs[k]).gt;
			} );
		} );
		gt = gt || (all_ge && igt);
		all_ge = all_ge && ige;
	}
}

#endif
