#ifndef TERMORD_HPP_
#define TERMORD_HPP_

#include"problem.hpp"
#include"template.hpp"

struct TermOrder {
	virtual ~TermOrder() = default;// to be able to make pointer of TermOrder 
	virtual int log() = 0;
	virtual void extend_sig( std::string const& f, Trs::Rank const& rank, bool heuristic ) = 0;
	virtual Smt::Solver& solver() = 0;
	virtual std::ostream& print_class( std::ostream& os ) = 0;
	virtual std::ostream& print_name( std::ostream& os ) = 0;
	virtual std::ostream& print_sym_info( std::ostream& os, std::string const& f ) = 0;
	virtual Smt::Compare order( Exp const& l, Exp const& r ) = 0;
	virtual Smt::Compare co_order( Exp const& s, Exp const& t ) = 0;
	virtual Smt::PostExp separate_if( Exp const& l, Exp const& r, Exp const& t ) {
		throw Error("#term-order","\"unsupported gt_if\"");
	}
	virtual Smt::PostExp separate( Exp const& s, Exp const& t ) = 0;
	virtual Smt::PostExp mono() = 0;
	/** collapsing: f(...x_i...) = x_i for some i */
	virtual Smt::PostExp fun_triv( std::string const& f ) = 0;
	/** inflationary: f(...x_i...) >= x_i */
	virtual Smt::PostExp arg_infl( std::string const& f, uint16_t i ) = 0;
	/** used: f(...x_i...) depends on x_i */
	virtual Smt::PostExp arg_used( std::string const& f, uint16_t i ) = 0;
	virtual void extend_sig( Trs::Sig const& sig, Set<std::string> const& heuristic ) {
		for( auto const& [f,rank] : sig ) {
			extend_sig(f,rank,heuristic.contains(f));
		}
	}
	Printable print_name() & {
		return Printable([&]( std::ostream& os )->std::ostream&{
			return print_name(os);
		});
	}
	Printable print_class() & {
		return Printable([&]( std::ostream& os )->std::ostream&{
			return print_class(os);
		});
	}
	Printable print_sym_info( std::string const& f ) {
		return Printable([&]( std::ostream& os )->std::ostream&{
			return print_sym_info(os,f);
		});
	}
	virtual std::ostream& print( std::ostream& os, Trs::Sig const& sig ) {
		os << '(' << print_class();
		for( auto [f,rank] : sig ) {
			os << "\n    (" << f << print_sym_info(f) << ')';
		}
		return os << ')';
	}
	Printable print( Trs::Sig const& sig ) & {
		return Printable([&]( std::ostream& os )->std::ostream&{
			return print(os,sig);
		});
	}

	static void test();
	static int log_of( Exp const& exp );
	static std::unique_ptr<TermOrder> make(
		Exp const& x,
		std::function<Smt::Solver()> const& default_smt,
		Smt::Sort const& default_sort,
		int default_log
	);
};

struct MemoizedTermOrder : TermOrder {
protected:
	OrdMap<std::pair<Exp,Exp>,Smt::Compare> _order_cache;
	OrdMap<std::pair<Exp,Exp>,Smt::Compare> _co_order_cache;
	virtual Smt::Compare order_inner( Exp const& l, Exp const& r ) = 0;
	virtual Smt::Compare co_order_inner( Exp const& s, Exp const& t ) = 0;
public:
	Smt::Compare order( Exp const& l, Exp const& r ) override {
		if( auto const& cache = _order_cache.find({l,r}) ) {
			return *cache;
		}
		auto ret = solver().let(order_inner(l,r));
		if( log() & PAIR ) {
			std::cerr << "; " << print_name() << ": (>=? " << l << ' ' << r << ") := " << ret << std::endl;
		}
		_order_cache.emplace(std::pair{l,r},ret);
		return ret;
	}
	Smt::Compare co_order( Exp const& s, Exp const& t ) override {
		if( auto const& cache = _co_order_cache.find({s,t}) ) {
			return *cache;
		}
		auto ret = solver().let(co_order_inner(s,t));
		if( log() & PAIR ) {
			std::cerr << "; " << print_name() << ": (>< " << s << ' ' << t << ") := " << ret << std::endl;
		}
		_co_order_cache.emplace(std::pair{s,t},ret);
		return ret;
	}
	static std::unique_ptr<MemoizedTermOrder> make( std::unique_ptr<TermOrder>&& ref );
private:
	struct _Wrapper;
};

struct MemoizedTermOrder::_Wrapper final : MemoizedTermOrder {
private:
	std::unique_ptr<TermOrder> _ref;
public:
	_Wrapper( std::unique_ptr<TermOrder>&& ref ) : _ref(std::move(ref)) {}
	Smt::Compare order_inner( Exp const& l, Exp const& r ) override {
		return _ref->order(l,r);
	}
	Smt::Compare co_order_inner( Exp const& s, Exp const& t ) override {
		return _ref->co_order(s,t);
	}
	Smt::PostExp separate( Exp const& s, Exp const& t ) override {
		return _ref->separate(s,t);
	}
	int log() override { return _ref->log(); }
	Smt::Solver& solver() override { return _ref->solver(); }
	void extend_sig( std::string const& f, Trs::Rank const& rank, bool heuristic ) override {
		_ref->extend_sig(f,rank,heuristic);
	}
	std::ostream& print_name( std::ostream& os ) override { return _ref->print_name(os); }
	std::ostream& print_class( std::ostream& os ) override { return _ref->print_class(os); }
	std::ostream& print_sym_info( std::ostream& os, std::string const& f ) override { return _ref->print_sym_info(os,f); }
	Smt::PostExp mono() override { return _ref->mono(); }
	Smt::PostExp fun_triv( std::string const& f ) override { return _ref->fun_triv(f); }
	Smt::PostExp arg_infl( std::string const& f, uint16_t i ) override { return _ref->arg_infl(f,i); }
	Smt::PostExp arg_used( std::string const& f, uint16_t i ) override { return _ref->arg_used(f,i); }
};

struct TrsOrder : TermOrder {
	virtual Smt::Compare rule_order( ruleId i, Trs::Term const& l, Trs::Term const& r ) = 0;
	virtual Smt::PostExp edge_co_order( ruleId i, ruleId j, Problem const& p ) = 0;
	virtual Smt::PostExp separate_edge( ruleId i, ruleId j, Problem const& p ) = 0;
	static std::unique_ptr<TrsOrder> make( std::unique_ptr<TermOrder>&& );
private:
	struct _Wrapper;
};
struct TrsOrder::_Wrapper final : TrsOrder {
private:
	std::unique_ptr<TermOrder> _ref;
	Map<ruleId,Smt::Compare> _rule_order_cache;
	Map<ruleId,Exp> _rename_cache;
	Map<ruleId,Map<ruleId,Smt::PostExp>> _edge_separate_cache;
	Map<ruleId,Map<ruleId,Smt::PostExp>> _bound_edge_cache;
public:
	_Wrapper( std::unique_ptr<TermOrder>&& ref ) : _ref(std::move(ref)) {}
	Smt::Compare rule_order( ruleId i, Trs::Term const& l, Trs::Term const& r ) override;
	Smt::PostExp edge_co_order( ruleId i, ruleId j, Problem const& p ) override;
	Smt::PostExp separate_edge( ruleId i, ruleId j, Problem const& p ) override;
	Exp get_renamed_lhs( ruleId i, Problem const& p ) {
		if( auto opt = _rename_cache.find(i) ) return *opt;
		auto src = ASSERTED(p.rules.find(i))->first.map(// register renamed the rhs of the source DP
			[&]( auto const& f ){ return p.sig.find(f) ? f : std::string("_")+f; }
		);
		return _rename_cache.emplace(i,src).first;
	}
	int log() override { return _ref->log(); }
	void extend_sig( std::string const& f, Trs::Rank const& rank, bool heuristic ) override {
		_ref->extend_sig(f,rank,heuristic);
	}
	Smt::Solver& solver() override { return _ref->solver(); }
	std::ostream& print_name( std::ostream& os ) override { return _ref->print_name(os); }
	std::ostream& print_class( std::ostream& os ) override { return _ref->print_class(os); }
	std::ostream& print_sym_info( std::ostream& os, std::string const& f ) override { return _ref->print_sym_info(os,f); }
	Smt::Compare order( Exp const& l, Exp const& r ) override { return _ref->order(l,r); }
	Smt::Compare co_order( Exp const& s, Exp const& t ) override { return _ref->co_order(s,t); }
	Smt::PostExp separate( Exp const& s, Exp const& t ) override { return _ref->separate(s,t); }
	Smt::PostExp mono() override { return _ref->mono(); }
	Smt::PostExp fun_triv( std::string const& f ) override { return _ref->fun_triv(f); }
	Smt::PostExp arg_infl( std::string const& f, uint16_t i ) override { return _ref->arg_infl(f,i); }
	Smt::PostExp arg_used( std::string const& f, uint16_t i ) override { return _ref->arg_used(f,i); }
};

struct UsableRuleOrder : TrsOrder {
	virtual Smt::PostExp term_used( Trs::Term const& t ) & = 0;
	virtual Smt::PostExp rule_used( ruleId i ) & = 0;
	static std::unique_ptr<UsableRuleOrder> make( std::unique_ptr<TrsOrder> const& org, Trs&& trs ) = delete;
	static std::unique_ptr<UsableRuleOrder> make( std::unique_ptr<TrsOrder>&& org, Trs const& trs, bool use );
private:
	struct _Wrapper;
	struct _Trivial;
};
struct UsableRuleOrder::_Wrapper final : UsableRuleOrder {
private:
	std::unique_ptr<TrsOrder> _ref;
	Trs const& _trs;
	Map<ruleId,Smt::PostExp> _usable_table;
public:
	_Wrapper( Trs const& trs, std::unique_ptr<TrsOrder> && org ) : _trs(trs), _ref(std::move(org)) {}
	Smt::PostExp term_used( Trs::Term const& t ) & override;
	Smt::PostExp rule_used( ruleId i ) & override;
	Smt::Compare rule_order( ruleId i, Trs::Term const& l, Trs::Term const& r ) override { return _ref->rule_order(i,l,r); }
	Smt::PostExp edge_co_order( ruleId i, ruleId j, Problem const& p ) override { return _ref->edge_co_order(i,j,p); }
	Smt::PostExp separate_edge( ruleId i, ruleId j, Problem const& p ) override { return _ref->separate_edge(i,j,p); }
	int log() override { return _ref->log(); }
	void extend_sig( std::string const& f, Trs::Rank const& rank, bool heuristic ) override {
		_ref->extend_sig(f,rank,heuristic);
	}
	Smt::Solver& solver() override { return _ref->solver(); }
	std::ostream& print_name( std::ostream& os ) override { return _ref->print_name(os); }
	std::ostream& print_class( std::ostream& os ) override { return _ref->print_class(os); }
	std::ostream& print_sym_info( std::ostream& os, std::string const& f ) override { return _ref->print_sym_info(os,f); }
	Smt::Compare order( Exp const& l, Exp const& r ) override { return _ref->order(l,r); }
	Smt::Compare co_order( Exp const& s, Exp const& t ) override { return _ref->co_order(s,t); }
	Smt::PostExp separate( Exp const& s, Exp const& t ) override { return _ref->separate(s,t); }
	Smt::PostExp mono() override { return _ref->mono(); }
	Smt::PostExp fun_triv( std::string const& f ) override { return _ref->fun_triv(f); }
	Smt::PostExp arg_infl( std::string const& f, uint16_t i ) override { return _ref->arg_infl(f,i); }
	Smt::PostExp arg_used( std::string const& f, uint16_t i ) override { return _ref->arg_used(f,i); }
};
struct UsableRuleOrder::_Trivial final : UsableRuleOrder {
private:
	std::unique_ptr<TrsOrder> _ref;
public:
	_Trivial( std::unique_ptr<TrsOrder> && org ) : _ref(std::move(org)) {}
	Smt::PostExp term_used( Trs::Term const& t ) & override {
		return true;
	}
	Smt::PostExp rule_used( ruleId i ) & override {
		return true;
	}
	Smt::Compare rule_order( ruleId i, Trs::Term const& l, Trs::Term const& r ) override {
		return _ref->rule_order(i,l,r);
	}
	Smt::PostExp separate( Exp const& s, Exp const& t ) override {
		return _ref->separate(s,t);
	}
	Smt::PostExp edge_co_order( ruleId i, ruleId j, Problem const& p ) override {
		return _ref->edge_co_order(i,j,p);
	}
	Smt::PostExp separate_edge( ruleId i, ruleId j, Problem const& p ) override { return _ref->separate_edge(i,j,p); }
	int log() override { return _ref->log(); }
	void extend_sig( std::string const& f, Trs::Rank const& rank, bool heuristic ) override {
		_ref->extend_sig(f,rank,heuristic);
	}
	Smt::Solver& solver() override { return _ref->solver(); }
	std::ostream& print_name( std::ostream& os ) override { return _ref->print_name(os); }
	std::ostream& print_class( std::ostream& os ) override { return _ref->print_class(os); }
	std::ostream& print_sym_info( std::ostream& os, std::string const& f ) override { return _ref->print_sym_info(os,f); }
	Smt::Compare order( Exp const& l, Exp const& r ) override { return _ref->order(l,r); }
	Smt::Compare co_order( Exp const& s, Exp const& t ) override { return _ref->co_order(s,t); }
	Smt::PostExp mono() override { return _ref->mono(); }
	Smt::PostExp fun_triv( std::string const& f ) override { return _ref->fun_triv(f); }
	Smt::PostExp arg_infl( std::string const& f, uint16_t i ) override { return _ref->arg_infl(f,i); }
	Smt::PostExp arg_used( std::string const& f, uint16_t i ) override { return _ref->arg_used(f,i); }
};

struct TrivOrder final : UsableRuleOrder {
private:
	Smt::Solver _solver;
public:
	TrivOrder( Smt::Solver&& sol ) : _solver(std::move(sol)) {}
	int log() override { return NONE; }
	void extend_sig( std::string const&, Trs::Rank const&, bool ) override {}
	void extend_sig( Trs::Sig const&, Set<std::string> const& ) override {}
	Smt::Solver& solver() override { return _solver; }
	Smt::Compare order( Exp const& l, Exp const& r ) override { return {true,false}; }
	Smt::Compare co_order( Exp const& s, Exp const& t ) override { return {true,false}; }
	Smt::PostExp separate( Exp const& s, Exp const& t ) override { return false; }
	std::ostream& print_name( std::ostream& os ) override { return os << "trivial-order"; }
	std::ostream& print_class( std::ostream& os ) override { return os << "trivial-order"; }
	std::ostream& print_sym_info( std::ostream& os, std::string const& sig ) override { return os; }
	Smt::PostExp mono() override { return true; }
	Smt::PostExp fun_triv( std::string const& f ) override { return true; }
	Smt::PostExp arg_infl( std::string const& f, uint16_t i ) override { return true; }
	Smt::PostExp arg_used( std::string const& f, uint16_t i ) override { return true; }
	Smt::Compare rule_order( ruleId, Trs::Term const&, Trs::Term const& ) override { return {true,false}; }
	Smt::PostExp edge_co_order( ruleId, ruleId, Problem const& ) override { return false; }
	Smt::PostExp separate_edge( ruleId, ruleId, Problem const& ) override { return false; }
	Smt::PostExp term_used( Trs::Term const& ) & override { return true; }
	Smt::PostExp rule_used( ruleId ) & override { return true; }
};

template<typename A>
struct DerivedTermOrder final : MemoizedTermOrder {
private:
	Smt::Solver _solver;
	OrdMap<Trs::Term,A> _evals;
	std::string _name;
	int _log;
	Algebra<std::string,A> const _intp;
public:
	Template::Deriver deriver;
	DerivedTermOrder(
		std::string name,
		Algebra<Template::Sym,A>&& org,
		Exp const& temp,
		Smt::Solver&& sol_,
		Smt::PostExp const& mono,
		int log_
	) : _solver(std::move(sol_)),
		_name(name),
		_log(log_),
		deriver(temp,_solver,mono,log_),
		_intp(deriver.derive(std::move(org))) {
	}
	DerivedTermOrder(
		std::string name,
		Algebra<Template::Sym,A> const& org,
		Exp const& temp,
		Smt::Solver&& sol_,
		Smt::PostExp const& mono,
		int log_
	) : _solver(std::move(sol_)),
		_name(name),
		_log(log_),
		deriver(temp,_solver,mono,log_),
		_intp(deriver.derive(org)) {
	}
	void extend_sig( std::string const& f, Trs::Rank const& rank, bool heuristic ) override {
		deriver.extend_sig(f,rank,heuristic);
		if( _log & DEBUG ) std::cerr << "; template " << f << ": " << *deriver.find(f) << std::endl;
	}
	A& intp( Trs::Term const& s ) & {
		if( auto const& ret = _evals.find(s) ) {
			return *ret;
		}
		auto [ret,fl] = _evals.emplace(s,_intp(s));
		ret.memoize(_solver);
		return ret;
	}
	Smt::Compare order_inner( Exp const& l, Exp const& r ) override {
		A const& li = intp(l), ri = intp(r);
		if( _log & PAIR ) {
			std::cerr << "; (>=? " << l << ' ' << r << ")\n;   " << li << "\n;   " << ri << std::endl;
		}
		return li.order(ri,_solver);
	}
	Smt::Compare co_order_inner( Exp const& s, Exp const& t ) override {
		A const& si = intp(s), ti = intp(t);
		if( _log & SEP ) {
			std::cerr << "; (< " << s << ' ' << t << ")\n;   " << si << "\n;   " << ti << std::endl;
		}
		return si.co_order(ti,_solver);
	}
	Smt::PostExp separate( Exp const& s, Exp const& t ) override {
		A const& si = intp(s), ti = intp(t);
		if( _log & SEP ) {
			std::cerr << "; (< " << s << ' ' << t << ")\n;   " << si << "\n;   " << ti << std::endl;
		}
		return si.separate(ti);
	}
	Smt::PostExp separate_if( Exp const& l, Exp const& r, Exp const& t ) override {
		Smt::PreExp cond = true;
		auto const& [f,rs] = *r;
		auto const& [g,ts] = *t;
		bool triv = true;
		if( f != g || rs.size() != ts.size() ) {
assert(false);
			return true;// edge is trivially infeasible
		}
		for( uint i = 0; i < rs.size(); i++ ) {
			A& ri = intp(rs[i]), ti = intp(ts[i]);
			if( ti.vars().empty() ) {// TODO: too strict?
				cond = cond && arg_used(f,i).imp(ri.ge_exp(ti));
				triv = false;
			}
		}
		if( triv ) return false;
		A& la = intp(l), ra = intp(r);
		auto sort = solver().logic().base_sort();
		return solver().expand(la.vars().quantify(sort, cond.imp(la.gt_exp(ra))));
	}
	std::ostream& print_class( std::ostream& os ) override {
		return os << "derived-order";
	}
	std::ostream& print_name( std::ostream& os ) override {
		return os << _name;
	};
	Smt::Solver& solver() override { return _solver; }
	std::ostream& print_sym_info( std::ostream& os, std::string const& f ) override {
		return os << " :intp " << Template::instantiator(solver())(*deriver.find(f));
	}
	int log() override { return _log; }
	Smt::PostExp mono() override { return deriver.mono; }
	Smt::PostExp fun_triv( std::string const& f ) override {
		return ASSERTED(deriver.sig.find(f))->triv;
	}
	Smt::PostExp arg_infl( std::string const& f, uint16_t i ) override {
		return ASSERTED(deriver.sig.find(f))->args[i].infl;
	}
	Smt::PostExp arg_used( std::string const& f, uint16_t i ) override {
		return ASSERTED(deriver.sig.find(f))->args[i].used;
	}
};

struct PathOrder final : MemoizedTermOrder {
	enum class Collapse {
		NONE, AUTO, FORCE,
	};
	struct Status {
		struct Straight {};
		struct Mapped {
			uint16_t post_arity;
			Collapse collapse;
		};
	private:
		Sum<Straight,Mapped> _sum;
	public:
		Status( Straight const& ) : _sum(Straight()) {}
		Status( Mapped b ) : _sum(b) {}
		bool is_straight() { return (bool)_sum.ref<Straight>(); }
		Opt<Mapped> mapped() {
			return _sum.ref<Mapped>();
		}
	};
	using StatusFun = std::function<Status( std::string const&, Trs::Rank const& )>;
	static StatusFun status_fun_of( Exp const& );
private:
	struct _SymInfo {
		uint16_t arity;
		Status status;
		Smt::PostExp prec, collapse, empty;
		std::function<Smt::PostExp(uint16_t,uint16_t)> map;// map(i,k) i-th argument is mapped to k-th place
		std::function<Smt::PostExp(uint16_t)> mapped;// mapped(i) if the i-th argument is mapped to some place
		std::function<Smt::PostExp(uint16_t)> occupied;// occupied(k) if some argument is mapped to k-th place
		std::function<Smt::PostExp(uint16_t)> used;// flags if the corresponding argument is used
	};
	std::unique_ptr<TermOrder> _weight;
	Map<std::string,_SymInfo> _info;
	Smt::PostExp _mono;// strict monotonicity flag
	int _log;
	StatusFun _status;
	using _TermComp = std::function<Smt::Compare(Exp const&,Exp const&)>;
	using _SymComp = std::function<Smt::Compare(_SymInfo const&,_SymInfo const&)>;
	Smt::Compare _compare_sub( _TermComp const& comp_sub, _TermComp const& weight, _SymComp const& prec, Exp const& l, Exp const& r );
	Smt::Compare _prec_compare( _SymInfo const& lf, _SymInfo const& rf ) {
		return { lf.prec.ge(rf.prec), lf.prec.gt(rf.prec) };
	}
	Smt::Compare _prec_co_order( _SymInfo const& lf, _SymInfo const& rf ) {
		return { !lf.prec.lt(rf.prec), !lf.prec.le(rf.prec) };
	}
public:
	PathOrder(
		std::unique_ptr<TermOrder>&& weight,
		StatusFun&& status,
		int log_
	);
	void extend_sig( std::string const& f, Trs::Rank const& rank, bool heuristic ) override;
	std::ostream& print_name( std::ostream& os ) override {
		return os << "path-order " << _weight->print_name();
	}
	std::ostream& print_class( std::ostream& os ) override {
		return os << "path-order " << _weight->print_class();
	}
	std::ostream& print_sym_info( std::ostream& os, std::string const& f ) override;
	Smt::Solver& solver() override {
		return _weight->solver();
	}
	Smt::Compare order_inner( Exp const& l, Exp const& r ) override {
		return _compare_sub(
			[&]( Exp const& l, Exp const& r ){ return order(l,r); },
			[&]( Exp const& l, Exp const& r ){ return _weight->order(l,r); },
			[&]( _SymInfo const& l, _SymInfo const& r ){ return _prec_compare(l,r); },
			l,r
		);
	}
	Smt::Compare co_order_inner( Exp const& l, Exp const& r ) override {
		return _compare_sub(
			[&]( Exp const& l, Exp const& r ){ return co_order(l,r); },
			[&]( Exp const& l, Exp const& r ){ return _weight->co_order(l,r); },
			[&]( _SymInfo const& l, _SymInfo const& r ){ return _prec_co_order(l,r); },
			l,r
		);
	}
	Smt::PostExp separate( Exp const& s, Exp const& t ) override {
		auto co = co_order(t,s);
		return co.ge && co.str;
	}
	int log() override { return _log; }
	Smt::PostExp mono() override { return _mono; }
	Smt::PostExp fun_triv( std::string const& f ) override {
		return ASSERTED(_info.find(f))->collapse;
	}
	Smt::PostExp arg_infl( std::string const& f, uint16_t i ) override {
		return ASSERTED(_info.find(f))->mapped(i);
	}
	Smt::PostExp arg_used( std::string const& f, uint16_t i ) override {
		return ASSERTED(_info.find(f))->used(i);
	}
};

extern Exp const SUM_SPEC, LPO_SPEC, LPO3_SPEC;

template<typename F, typename I>
Smt::Compare lex_order( F const& comp, I const& lit, I const& lend, I const& rit, I const& rend ) {
	if( lit == lend ) {// lhs is empty
		if( rit == rend ) return {true,false};// rhs is also empty
		return {false,true};// rhs is longer
	}
	if( rit == rend ) return {true,true};// lhs is longer: >= is enough for >
	auto const& [ge,nle] = comp(*lit,*rit);
	auto const& [rest_ge,rest_nle] = lex_order(comp,lit+1,lend,rit+1,rend);
	return {ge && (nle || rest_ge), nle || rest_nle};
}
template<typename F, typename C>
Smt::Compare lex_order( F const& comp, C const& ls, C const& rs ) {
	return lex_order(comp,ls.begin(),ls.end(),rs.begin(),rs.end());
}

template<typename F, typename T>
Smt::Compare mapped_lex_order(
	Smt::Solver& solver,
	F const& comp,
	std::function<Smt::PostExp(uint16_t)> const& locc,// occupancy of post-positions. locc(i+1) => loc(i) is assumed 
	std::function<Smt::PostExp(uint16_t)> const& rocc,
	std::function<Smt::PostExp(uint16_t,uint16_t)> const& lmap,
	std::function<Smt::PostExp(uint16_t,uint16_t)> const& rmap,
	std::vector<T> const& ls,
	std::vector<T> const& rs,
	uint16_t k = 0
) {
	auto lin = ls.size();
	auto rin = rs.size();
	auto const& locck = locc(k);
	auto const& rocck = rocc(k);
	if( locck == false ) {// lhs has no more post arguments
		return { !rocck, false };// ge iff rhs is empty, and never strict
	}
	if( rocck == false ) {// rhs has no more post arguments
		return {true,locck};// always ge, and strict if lhs is nonempty
	}
	auto k_ge =// if rhs survives, lhs must cover it
		Smt::PostExp::conj( 0, rin, [&]( uint16_t const& j ){
			return rmap(j,k).imp(
				Smt::PostExp::disj( 0, lin, [&]( uint16_t const& i ){// [l] >= [r]
					return lmap(i,k) && comp(ls[i],rs[j]).ge;
				} )
			);
		} );
	auto k_str = solver.let( Smt::BOOL,
		Smt::PostExp::disj( 0, lin, [&]( uint16_t const& i ){
			return lmap(i,k) && Smt::PostExp::conj( 0, rin, [&]( uint16_t const& j ){// notice: [l] > []
				return rmap(j,k).imp(comp(ls[i],rs[j]).str);
			} );
		} )
	);
	auto [rest_ge,rest_str] = mapped_lex_order(solver,comp,locc,rocc,lmap,rmap,ls,rs,k+1);
	return {
		k_ge && (k_str || rest_ge),// l_0,l_1... ≥ r_0,r_1... <==> l_0 ≥ r_0 && (l_0 ≠ r_0 || l_1... ≥ r_1...)
		k_str || rest_str
	};
}

template<typename F, typename T>
Smt::PostExp mapped_lex_separate(
	Smt::Solver& solver,
	F const& sep,
	std::function<Smt::PostExp(uint16_t)> const& locc,// occupancy of post-positions. locc(i+1) => loc(i) is assumed 
	std::function<Smt::PostExp(uint16_t)> const& rocc,
	std::function<Smt::PostExp(uint16_t,uint16_t)> const& lmap,
	std::function<Smt::PostExp(uint16_t,uint16_t)> const& rmap,
	std::vector<T> const& ls,
	std::vector<T> const& rs,
	uint16_t k = 0
) {
	auto lin = ls.size();
	auto rin = rs.size();
	auto const& locck = locc(k);
	auto const& rocck = rocc(k);
	if( locck == false ) {// lhs has no more post arguments
		return rocck;// separated iff rhs survives
	}
	if( rocck == false ) {// rhs has no more post arguments
		return locck;// separated iff lhs survives
	}
	auto k_sep = solver.let( Smt::BOOL,
		Smt::PostExp::disj( 0, lin, [&]( uint16_t const& i ){
			return lmap(i,k) && Smt::PostExp::conj( 0, rin, [&]( uint16_t const& j ){// notice: [l] > []
				return rmap(j,k).imp(sep(ls[i],rs[j]));
			} );
		} )
	);
	return k_sep || mapped_lex_separate(solver,sep,locc,rocc,lmap,rmap,ls,rs,k+1);
}

#endif
