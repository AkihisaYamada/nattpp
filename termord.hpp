#ifndef TERMORD_HPP_
#define TERMORD_HPP_

#include"trs.hpp"
#include"template.hpp"

struct TermOrder {
	virtual ~TermOrder() = default;// to be able to make pointer of TermOrder 
	enum { NONE = 0, RULE = 1 << 1, PAIR = 1 << 2, DEBUG = 1 << 3 };
	virtual int log() = 0;
	virtual void extend_sig( std::string const& f, Trs::Rank const& rank ) = 0;
	virtual Smt::Solver& solver() = 0;
	virtual std::ostream& print_name( std::ostream& os ) = 0;
	virtual std::ostream& print_sym_info( std::ostream& os, std::string const& f ) = 0;
	virtual Smt::Compare compare( Exp const& l, Exp const& r ) = 0;
	virtual Smt::PostExp mono() = 0;
	virtual Smt::PostExp arg_infl( std::string const& f, size_t i ) = 0;
	virtual Smt::PostExp arg_used( std::string const& f, size_t i ) = 0;
	virtual void extend_sig( Trs::Sig const& sig ) {
		for( auto const& [f,rank] : sig ) {
			extend_sig(f,rank);
		}
	}
	Printable print_name() & {
		return Printable([&]( std::ostream& os )->std::ostream&{
			return print_name(os);
		});
	}
	Printable print_sym_info( std::string const& f ) {
		return Printable([&]( std::ostream& os )->std::ostream&{
			return print_sym_info(os,f);
		});
	}
	virtual std::ostream& print( std::ostream& os, Trs::Sig const& sig ) {
		os << '(' << print_name();
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
	OrdMap<std::pair<Term<std::string>,Term<std::string>>,Smt::Compare> _table;
	virtual Smt::Compare compare_inner( Exp const& l, Exp const& r ) = 0;
public:
	Smt::Compare compare( Exp const& l, Exp const& r ) final override {
		if( auto const& opt = _table.find({l,r}) ) {
			return *opt;
		}
		auto [ge,gt] = compare_inner(l,r);
		auto ret = Smt::Compare(solver().let(Smt::BOOL,ge),solver().let(Smt::BOOL,gt));
		if( log() & PAIR ) {
			std::cerr << "; " << l << " <=> " << r << " = " << ret << std::endl;
		}
		_table.emplace(std::pair{l,r},ret);
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
	Smt::Compare compare_inner( Exp const& l, Exp const& r ) override {
		return _ref->compare(l,r);
	}
	int log() override { return _ref->log(); }
	Smt::Solver& solver() override { return _ref->solver(); }
	void extend_sig( std::string const& f, Trs::Rank const& rank ) override { _ref->extend_sig(f,rank); }
	std::ostream& print_name( std::ostream& os ) override { return _ref->print_name(os); }
	std::ostream& print_sym_info( std::ostream& os, std::string const& f ) override { return _ref->print_sym_info(os,f); }
	Smt::PostExp mono() override { return _ref->mono(); }
	Smt::PostExp arg_infl( std::string const& f, size_t i ) override { return _ref->arg_infl(f,i); }
	Smt::PostExp arg_used( std::string const& f, size_t i ) override { return _ref->arg_used(f,i); }
};

struct TrsOrder : TermOrder {
	virtual Smt::Compare rule_compare( size_t i, Trs::Term const& l, Trs::Term const& r ) = 0;
	static std::unique_ptr<TrsOrder> make( std::unique_ptr<TermOrder>&& );
private:
	struct _Wrapper;
};
struct TrsOrder::_Wrapper final : TrsOrder {
private:
	std::unique_ptr<TermOrder> _ref;
	Map<size_t,Smt::Compare> _rule_order_table;
public:
	_Wrapper( std::unique_ptr<TermOrder>&& ref ) : _ref(std::move(ref)) {}
	Smt::Compare rule_compare( size_t i, Trs::Term const& l, Trs::Term const& r ) override;
	int log() override { return _ref->log(); }
	void extend_sig( std::string const& f, Trs::Rank const& rank ) override { _ref->extend_sig(f,rank); }
	Smt::Solver& solver() override { return _ref->solver(); }
	std::ostream& print_name( std::ostream& os ) override { return _ref->print_name(os); }
	std::ostream& print_sym_info( std::ostream& os, std::string const& f ) override { return _ref->print_sym_info(os,f); }
	Smt::Compare compare( Exp const& l, Exp const& r ) override { return _ref->compare(l,r); }
	Smt::PostExp mono() override { return _ref->mono(); }
	Smt::PostExp arg_infl( std::string const& f, size_t i ) override { return _ref->arg_infl(f,i); }
	Smt::PostExp arg_used( std::string const& f, size_t i ) override { return _ref->arg_used(f,i); }
};

struct UsableRuleOrder : TrsOrder {
	virtual Smt::PostExp term_used( Trs::Term const& t ) & = 0;
	virtual Smt::PostExp rule_used( size_t i ) & = 0;
	static std::unique_ptr<UsableRuleOrder> make( Trs&& trs, std::unique_ptr<TrsOrder> const& org ) = delete;
	static std::unique_ptr<UsableRuleOrder> make( Trs const& trs, std::unique_ptr<TrsOrder>&& org );
private:
	struct _Wrapper;
};
struct UsableRuleOrder::_Wrapper final : UsableRuleOrder {
private:
	std::unique_ptr<TrsOrder> _ref;
	Trs const& _trs;
	Map<size_t,Smt::PostExp> _usable_table;
public:
	_Wrapper( Trs const& trs, std::unique_ptr<TrsOrder> && org ) : _trs(trs), _ref(std::move(org)) {}
	Smt::PostExp term_used( Trs::Term const& t ) & override;
	Smt::PostExp rule_used( size_t i ) & override;
	Smt::Compare rule_compare( size_t i, Trs::Term const& l, Trs::Term const& r ) override { return _ref->rule_compare(i,l,r); }
	int log() final override { return _ref->log(); }
	void extend_sig( std::string const& f, Trs::Rank const& rank ) override { _ref->extend_sig(f,rank); }
	Smt::Solver& solver() final override { return _ref->solver(); }
	std::ostream& print_name( std::ostream& os ) final override { return _ref->print_name(os); }
	std::ostream& print_sym_info( std::ostream& os, std::string const& f ) final override { return _ref->print_sym_info(os,f); }
	Smt::Compare compare( Exp const& l, Exp const& r ) final override { return _ref->compare(l,r); }
	Smt::PostExp mono() final override { return _ref->mono(); }
	Smt::PostExp arg_infl( std::string const& f, size_t i ) final override { return _ref->arg_infl(f,i); }
	Smt::PostExp arg_used( std::string const& f, size_t i ) final override { return _ref->arg_used(f,i); }
};

struct TrivOrder final : UsableRuleOrder {
private:
	Smt::Solver _solver;
public:
	TrivOrder( Smt::Solver&& sol ) : _solver(std::move(sol)) {}
	int log() override { return NONE; }
	void extend_sig( std::string const& f, Trs::Rank const& rank ) override {}
	void extend_sig( Trs::Sig const& sig ) override {}
	Smt::Solver& solver() override { return _solver; }
	Smt::Compare compare( Exp const& l, Exp const& r ) override {
		return {true,false};
	};
	std::ostream& print_name( std::ostream& os ) override {
		return os << "trivial-order";
	};
	std::ostream& print_sym_info( std::ostream& os, std::string const& sig ) override {
		return os;
	}
	Smt::PostExp mono() override {
		return true;
	}
	Smt::PostExp arg_infl( std::string const& f, size_t i ) override {
		return true;
	}
	Smt::PostExp arg_used( std::string const& f, size_t i ) override {
		return true;
	}
	Smt::Compare rule_compare( size_t, Trs::Term const&, Trs::Term const& ) override {
		return {true,false};
	}
	Smt::PostExp term_used( Trs::Term const& t ) & override {
		return true;
	}
	Smt::PostExp rule_used( size_t i ) & override {
		return true;
	}
};

template<typename A>
struct DerivedTermOrder final : MemoizedTermOrder {
private:
	Smt::Solver _solver;
	int _log;
public:
	Template::Deriver deriver;
	Algebra<std::string,A> const intp;
	DerivedTermOrder(
		Exp const& temp,
		Smt::Solver&& sol_,
		int log_ = NONE
	) : _solver(std::move(sol_)),
		_log(log_),
		deriver(temp,_solver),
		intp(deriver.derive(A::ALGEBRA)) {
	}
	void extend_sig( std::string const& f, Trs::Rank const& rank ) override {
		deriver.extend_sig(f,rank);
	}

	Smt::Compare compare_inner( Exp const& l, Exp const& r ) override {
		return order(intp(l),intp(r),_solver);
	}
	std::ostream& print_name( std::ostream& os ) override {
		return os << "derived-order";
	};
	Smt::Solver& solver() override {
		return _solver;
	}
	std::ostream& print_sym_info( std::ostream& os, std::string const& f ) override {
		return os << " :intp " << Template::instantiator(solver())(*deriver.find(f));
	}
	int log() override {
		return _log;
	}
	Smt::PostExp mono() override {
		return deriver.mono;
	}
	Smt::PostExp arg_infl( std::string const& f, size_t i ) override {
		return (*ASSERTED(deriver.sig.find(f)))[i].inflationary;
	}
	Smt::PostExp arg_used( std::string const& f, size_t i ) override {
		return (*ASSERTED(deriver.sig.find(f)))[i].used;
	}
};

struct PathOrder final : MemoizedTermOrder {
private:
	struct _SymInfo {
		Smt::PostExp prec;
		size_t arity;
		size_t post_arity;// arity after argument rearrangement
		std::function<Smt::PostExp(size_t,size_t)> map;// map(i,j) i-th argument is mapped to j-th position
		std::function<Smt::PostExp(size_t)> mapped;// flags if the corresponding argument is mapped
		std::function<Smt::PostExp(size_t)> used;// flags if the corresponding argument is used
	};
	std::unique_ptr<TermOrder> _weight;
	Map<std::string,_SymInfo> _info;
	Smt::PostExp _mono;// strict monotonicity flag
	int _log;
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
	};
	struct StatusFun {
		std::function<Status(Trs::Rank const&)> fun;
		StatusFun( std::function<Status(Trs::Rank const&)>&& arg ) : fun(std::move(arg)) {}
		static StatusFun of( Exp const& );
	};
private:
	StatusFun _status;
public:
	PathOrder(
		std::unique_ptr<TermOrder>&& weight,
		StatusFun&& status,
		int log_
	);
	void extend_sig( std::string const& f, Trs::Rank const& rank ) override;
	std::ostream& print_name( std::ostream& os ) override {
		return os << "path-order " << _weight->print_name();
	}
	std::ostream& print_sym_info( std::ostream& os, std::string const& f ) override;
	Smt::Solver& solver() override {
		return _weight->solver();
	}
	Smt::Compare compare_inner( Exp const& l, Exp const& r ) override;
	int log() override { return _log; }
	Smt::PostExp mono() override { return _mono; }
	Smt::PostExp arg_infl( std::string const& f, size_t i ) override {
		return ASSERTED(_info.find(f))->mapped(i);
	}
	Smt::PostExp arg_used( std::string const& f, size_t i ) override {
		return ASSERTED(_info.find(f))->used(i);
	}
};

extern Exp const SUM_SPEC, LPO_SPEC, LPO3_SPEC;

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
	std::function<Smt::PostExp(size_t,size_t)> const& lmap,
	std::function<Smt::PostExp(size_t,size_t)> const& rmap,
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
			return lmap(i,k) && Smt::disj( 0, rin, [&]( size_t const& j ){
				return rmap(j,k) && comp(ls[i],rs[j]).ge;
			} );
		} );
		auto igt = Smt::disj( 0, lin, [&]( size_t const& i ){
			return lmap(i,k) && Smt::disj( 0, rin, [&]( size_t const& j ){
				return rmap(j,k) && comp(ls[i],rs[j]).gt;
			} );
		} );
		gt = gt || (all_ge && igt);
		all_ge = all_ge && ige;
	}
}

#endif
