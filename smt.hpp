#ifndef _SMT_HPP
#define _SMT_HPP

#include<iostream>
#include <numeric>
#include"set.hpp"
#include"algebra.hpp"
#include"proc.hpp"

class Smt {
public:
	struct Error : ::Error {
		using ::Error::Error;
	};
	class BaseSort {
		std::string _name;
		BaseSort( char const* name ) : _name(name) {}
		BaseSort( std::string const& name ) : _name(name) {}
	public:
		std::string name() && { return std::move(_name); }
		std::string const& name() const& { return _name; }
		static BaseSort of( Exp const& );
		friend Smt;
		bool operator==( Smt::BaseSort const& y ) const& = default;
	};
	class Rat {
		int _numen, _denom;
	public:
		Rat( int i ) : _numen(i), _denom(1) {}
		Rat( int numen, int denom ) {
			int gcd = std::gcd(numen,denom);
			_numen = numen/gcd;
			_denom = denom/gcd;
		}
		friend bool operator==( Rat const&, Rat const& ) = default;
		int numen() const { return _numen; }
		int denom() const { return _denom; }
		friend Rat operator+( Rat const& x, Rat const& y ) {
			int n = std::gcd(x._denom,y._denom);
			int m = y._denom/n;
			return Rat( x._numen * m + y._numen * (x._denom / n), x._denom * m );
		}
		friend Rat& operator+=( Rat& x, Rat const& y ) {
			return x = x + y;
		}
		friend Rat& operator*=( Rat& x, Rat const& y ) {
			int ngcd = std::gcd(x._numen,y._denom);
			int dgcd = std::gcd(x._denom,y._numen);
			x._numen = x._numen / ngcd * (y._numen / dgcd);
			x._denom = x._denom / dgcd * (y._denom / ngcd);
			return x;
		}
		friend Rat operator*( Rat const& x, Rat const& y ) {
			Rat z = x;
			return z+=y;
		}
		friend auto operator<=>(Rat const& x, Rat const& y) {
			return (long long)x._numen * y._denom <=> (long long)y._numen * x._denom;
		}
	};
	class Val {
		Sum<int,Rat> _sum;
	public:
		Val( int i ) : _sum(i) {}
		Val( Rat r ) : _sum(r) {}
		Opt<int const&> is_int() const& { return _sum.ref<int>(); }
		Opt<int&> is_int()& { return _sum.ref<int>(); }
		Opt<Rat const&> is_rat() const& { return _sum.ref<Rat>(); }
		Opt<Rat&> is_rat()& { return _sum.ref<Rat>(); }
		friend bool operator==( Val const& x, Val const& y ) {
			if( auto const& xi = x.is_int() ) {
				if( auto const& yi = y.is_int() ) {
					return *xi == *yi;
				} else if( auto const& yr = y.is_rat() ) {
					return yr->denom() == 1 && *xi == yr->numen();
				}
			} else if( auto const& xr = x.is_rat() ) {
				if( auto const& yi = y.is_int() ) {
					return xr->denom() == 1 && xr->numen() == *yi;
				} else if( auto const& yr = y.is_rat() ) {
					return *xr == *yr;
				}
			}
			assert(false);
		}
		friend auto operator<=>( Val const& x, Val const& y ) {
			if( auto xi = x.is_int() ) {
				if( auto yi = y.is_int() ) {
					return *xi <=> *yi;
				} else if( auto yr = y.is_rat() ) {
					return *xi <=> *yr;
				}
			} else if( auto xr = x.is_rat() ) {
				if( auto yi = y.is_int() ) {
					return *xr <=> *yi;
				} else if( auto yr = y.is_rat() ) {
					return *xr <=> *yr;
				}
			}
			assert(false);
		}
		friend Val& operator+=( Val& x, Val const& y ) {
			if( auto xi = x._sum.ref<int>() ) {
				if( auto yi = y._sum.ref<int>() ) {
					*xi += *yi;
					return x;
				}
				if( auto yr = y._sum.ref<Rat>() ) {
					return x = *xi + *yr;
				}
			} else if( auto xr = x._sum.ref<Rat>() ) {
				if( auto yi = y._sum.ref<int>() ) {
					*xr += *yi;
					return x;
				}
				if( auto yr = y._sum.ref<Rat>() ) {
					*xr += *yr;
					return x;
				}
			}
			assert(false);
		}
		friend Val operator+( Val x, Val const& y ) {
			return x+=y;
		}
		friend Val& operator*=( Val& x, Val const& y ) {
			if( auto xi = x._sum.ref<int>() ) {
				if( auto yi = y._sum.ref<int>() ) {
					*xi *= *yi;
					return x;
				}
				if( auto yr = y._sum.ref<Rat>() ) {
					return x = *xi * *yr;
				}
			} else if( auto xr = x._sum.ref<Rat>() ) {
				if( auto yi = y._sum.ref<int>() ) {
					*xr *= *yi;
					return x;
				}
				if( auto yr = y._sum.ref<Rat>() ) {
					*xr *= *yr;
					return x;
				}
			}
			assert(false);
		}
		friend Val operator*( Val x, Val const& y ) {
			return x *= y;
		}
	};
	class Sort {
		struct _Cons;
		Sum<BaseSort,Ref<_Cons>> _un;
	public:
		Sort( BaseSort const& base ) : _un(base) {}
		Sort( Sort const& x, Sort const& y );
		Opt<BaseSort const&> base() const & {
			return _un.ref<BaseSort>();
		}
		OptRef<_Cons> cons() const &;
		static Sort of( Exp const& x );
	};

	static BaseSort const BOOL, INT, REAL;
	class Logic {
		bool _linear;
		char const* _str;
		BaseSort const& _base_sort;
		friend Smt;
		Logic( char const* str, bool linear, BaseSort const& base_sort ) : _str(str), _linear(linear), _base_sort(base_sort) {}
	public:
		bool linear() const { return _linear; }
		BaseSort const& base_sort() const& { return _base_sort; }
		static Logic of( Exp const& );
	};
	static Logic const QF_LIA, QF_LRA, LIA, LRA, QF_NIA, QF_NRA, NIA, NRA;
	using Fun = Sum<std::string,Val>;
	static std::string const TRUE_F, FALSE_F, AND, OR, NOT, IMP, ITE, ADD, MUL, EQ, GE, LE, GT, CONS, CAR, CDR, LIST, NTH, MAX;
	static Set<std::string> const FUNS;
	static Opt<Fun> is_fun( std::string const& sym ) {
		return FUNS.find(sym) >>= []( auto const& str ){ return Opt<Fun>::make(str); };
	}
	class PostExp;
	static PostExp const TRUE, FALSE;
	class PostExp {
		friend Smt;
		Term<Fun> _term;
		PostExp( Term<Fun> const& term ) : _term(term) {}
	public:
		PostExp( PostExp const& ) = default;
		PostExp( PostExp && ) = default;
		PostExp( bool b ) : _term( b ? TRUE : FALSE ) {}
		PostExp( int i ) : _term(Val(i)) {}
		PostExp( unsigned int i ) : _term(Val(i)) {}
		PostExp( Val v ) : _term(v) {}
		operator Term<Fun> const&() const& {
			return _term;
		}
		PostExp& operator=( PostExp const& other ) & {
			_term = other._term;
			return *this;
		}
		PostExp& operator=( PostExp&& other ) & {
			_term = std::move(other._term);
			return *this;
		}
		bool operator==( PostExp const& other ) const {
			return _term == other._term;
		}
		bool operator!=( PostExp const& other ) const {
			return _term != other._term;
		}
		Opt<std::string> is_app() && { return std::move(_term).fun().ref<std::string>(); }
		Opt<std::string const&> is_app() const& { return _term.fun().ref<std::string>(); }
		Opt<Val> is_val() && { return std::move(_term).fun().ref<Val>(); }
		Opt<Val const&> is_val() const & { return _term.fun().ref<Val>(); }
		Val as_val() const& {
			auto opt = is_val();
			assert(opt);
			return *opt;
		}
		Opt<bool> is_bool() const & {
			if( auto str = _term.fun().ref<std::string>() ) {
				if( *str == "true" ) return {true};
				if( *str == "false" ) return {false};
			}
			return {};
		}
		Opt<std::tuple<PostExp,PostExp,PostExp>> is_ite() const &;
public:
		PostExp conj( PostExp const& y ) const &;
		PostExp disj( PostExp const& y ) const &;
		PostExp imp( PostExp const& y ) const;
		PostExp operator!() const;
		friend PostExp& operator+=( PostExp& x, PostExp const& y );
		PostExp& mul_eq( PostExp const&, bool linear ) &;
		PostExp cons( PostExp const& y ) const {
			return Term<Fun>(CONS,*this,y);
		}
		Exp exp() const;
	};
	static PostExp disj( auto i, auto const& end, auto const& f ) {
		std::vector<Term<Fun>> ds;
		for( ; i != end; i++ ) {
			PostExp const& fx = f(i);
			if( fx == TRUE ) return TRUE;
			if( fx != FALSE ) ds.push_back(fx._term);
		}
		switch( ds.size() ) {
		case 0: return FALSE;
		case 1: return PostExp(ds[0]);
		}
		return PostExp(app(OR,std::move(ds)));
	}
	template<typename C>
	static PostExp disj( C const& xs, auto const& f ) {
		return disj( xs.begin(), xs.end(), [&]( typename C::const_iterator const& i ){ return f(*i); } );
	}
	template<typename C>
	static PostExp disj( C const& xs ) {
		return disj( xs, []( auto const& x ){ return x; } );
	}

	static PostExp conj( auto i, auto const& end, auto const& f ) {
		std::vector<Term<Fun>> cs;
		for( ; i != end; i++ ) {
			PostExp const& fx = f(i);
			if( fx == FALSE ) return FALSE;
			if( fx != TRUE ) cs.push_back(fx._term);
		}
		switch( cs.size() ) {
		case 0: return TRUE;
		case 1: return cs[0];
		}
		return app(AND,std::move(cs));
	}
	template<typename C>
	static PostExp conj( C const& xs, auto const& f ) {
		return conj( xs.begin(), xs.end(), [&]( auto const& i ){ return f(*i); } );
	}
	static PostExp car( PostExp const& arg );
	static PostExp cdr( PostExp const& arg );
	struct Compare {
		PostExp ge, gt;
	};
	class PreExp {
		friend Smt;
		class App;
		class Let;
		using Lazy = std::function<PreExp()>;
		Sum<PostExp,Ref<App>,Ref<Let>,Lazy> _un;
		explicit PreExp( Sort const& sort, PreExp const& val, std::function<PreExp(PostExp const&)> body ) :
			_un(Ref<Let>::make(val,sort,body)) {}
		explicit PreExp( std::string const& fun, std::vector<PreExp>&& args ) :
			_un(Ref<App>::make(fun,std::move(args))) {
}
	public:
		PreExp( PostExp const& e ) : _un(e) {}
		template<typename T>
			requires std::is_constructible_v<Lazy,T>
		PreExp( T const& lazy ) : _un(std::in_place_type<Lazy>,lazy) {}
		PreExp operator!() const {
			return PreExp(NOT,{*this});
		}
		Opt<PostExp> is_post() && {
			return std::move(_un).ref<PostExp>();
		}
		Opt<PostExp const&> is_post() const& {
			return _un.ref<PostExp>();
		}
		OptRef<App> is_app() && {
			return std::move(_un).ref<Ref<App>>() >>= []( auto ref )->OptRef<App>{ return ref; };
		}
		Opt<App const&> is_app() const& {
			return _un.ref<Ref<App>>() >>= []( auto ref )->Opt<App const&>{ return {*ref}; };
		}
		OptRef<Let> is_let() && {
			return std::move(_un).ref<Ref<Let>>() >>= []( auto ref )->OptRef<Let>{ return ref; };
		}
		Opt<Let const&> is_let() const& {
			return _un.ref<Ref<Let>>() >>= []( auto ref )->Opt<Let const&>{ return {*ref}; };
		}
		Opt<Lazy> is_lazy() && {
			return {std::move(_un).ref<Lazy>()};
		}
		Opt<Lazy const&> is_lazy() const& {
			return _un.ref<Lazy>();
		}
		PreExp conj( PreExp const& y ) const {
			return PreExp(AND,{*this,y});
		}
		PreExp disj( PreExp const& y ) const {
			return PreExp(OR,{*this,y});
		}
		PreExp add( PreExp const& y ) const {
			return PreExp(ADD,{*this,y});
		}
		friend PreExp& operator+=( PreExp& x, PreExp const& y ) {
			return x = x.add(y);
		}
		PreExp mul( PreExp const& y ) const {
			return PreExp(MUL,{*this,y});
		}
		friend PreExp& operator*=( PreExp& x, PreExp const& y ) {
			return x = x.mul(y);
		}
		PreExp cons( PreExp const& y ) const {
			return PreExp(CONS,{*this,y});
		}
		friend inline bool operator==( Smt::PreExp const& x, Smt::PostExp const& y ) {
			if( auto const& o = x._un.ref<PostExp>() ) {
				return *o == y;
			}
			return false;
		}
		friend inline bool operator==( Smt::PostExp const& x, Smt::PreExp const& y ) {
			if( auto const& o = y._un.ref<PostExp>() ) {
				return x == *o;
			}
			return false;
		}
	};
	static PreExp car( PreExp const& arg ) {
		return PreExp{CAR,{arg}};
	}
	static PreExp cdr( PreExp const& arg ) {
		return PreExp{CDR,{arg}};
	}
	static PostExp eq( PostExp const& x, PostExp const& y );
	static PreExp eq( PreExp const& x, PreExp const& y ) {
		return PreExp(EQ,{x,y});
	}
	static PostExp le( PostExp const& x, PostExp const& y );
	static PreExp le( PreExp const& x, PreExp const& y ) {
		return PreExp(LE,{x,y});
	}
	static PostExp ge( PostExp const& x, PostExp const& y );
	static PreExp ge( PreExp const& x, PreExp const& y ) {
		return PreExp(GE,{x,y});
	}
	static PostExp gt( PostExp const& x, PostExp const& y );
	static PreExp gt( PreExp const& x, PreExp const& y ) {
		return PreExp(GT,{x,y});
	}
	static PostExp ite( PostExp const& i, PostExp const& t, PostExp const& e );
	static PreExp ite( PreExp const& i, PreExp const& t, PreExp const& e ) {
		return PreExp(ITE,{i,t,e});
	}
	static PreExp ite( PreExp const& i, PreExp const& t, PostExp const& e ) {
		return PreExp(ITE,{i,t,e});
	}
	static PreExp ite( PreExp const& i, PostExp const& t, PreExp const& e ) {
		return PreExp(ITE,{i,t,e});
	}
	static PreExp ite( PreExp const& i, PostExp const& t, PostExp const& e ) {
		return PreExp(ITE,{i,t,e});
	}
	static PostExp mul( PostExp x, PostExp const& y, bool linear ) {
		return x.mul_eq(y,linear);
	}
private:
	struct _If2 {
		PreExp const& i;
		PreExp const& t;
		PreExp operator^( PreExp const& e ) const {
			return PreExp(ITE,{i,t,e});
		}
	};
public:
	class If {
		PreExp const& i;
	public:
		If( PreExp const& i ) : i(i) {}
		_If2 operator^( PreExp const& t ) const {
			return {i,t};
		}
	};
	class Let {
		Sort const& sort;
		PreExp const& val;
		Let( Let const& ) = delete;
	public:
		explicit Let( Sort const& sort, PreExp const& val ) : sort(sort), val(val) {}
		PreExp operator^( std::function<PreExp(PostExp const&)> body ) const {
			return PreExp(sort,val,body);
		}
	};
	static Algebra<std::string,Smt::PreExp> const ALGEBRA;
	class Reader : public ::Reader {
	public:
		using ::Reader::Reader;
		PostExp read_post_exp();
	};
	class Solver {
		friend Smt;
		enum { UNKNOWN, SOLVING, SAT, UNSAT } _status;
		std::unique_ptr<Proc> _proc;
		Reader _reader;
		size_t _var_count;
		Logic _logic;
		Solver( std::unique_ptr<Proc>&& proc, Logic const& logic );
		Solver( Solver const& other ) = delete;
		Solver& operator=( Solver const& other ) = delete;
		std::string _make_fresh() &;
	public:
		Solver( Solver&& other ) = default;
		Logic const& logic() const& { return _logic; }
		PostExp declare_const( std::string const& name, BaseSort const& sort ) &;
		PostExp declare_fresh( BaseSort const& sort ) {
			std::string ret = _make_fresh();
			return declare_const(ret,sort);
		}
		PostExp expand( PreExp const& p );
		PostExp define_fun(
			std::string const& name,
			std::initializer_list<std::pair<std::string,std::string>> const& params,
			BaseSort const& sort,
			PostExp const& body
		) &;
		template<class T>
			requires (!std::is_convertible_v<T,PostExp> && std::is_convertible_v<T,PreExp>)
		PostExp define_fun(
			std::string const& name,
			std::initializer_list<std::pair<std::string,std::string>> const& params,
			BaseSort const& sort,
			T const& body
		) & {
			return define_fun(name,params,sort,expand(body));
		}
		PostExp let( Sort const& sort, PostExp const& val ) &;
		PostExp let( PostExp const& val ) & {
			return let(_logic._base_sort,val);
		}
		PostExp let( Sort const& sort, PreExp const& val ) & {
			return let(sort,expand(val));
		}
		PostExp let( PreExp const& val ) & {
			return let(_logic._base_sort,expand(val));
		}
		Solver& ass( PostExp const& e ) &;
		Solver& ass( PreExp const& p ) & {
			return ass(expand(p));
		}
		Solver& push() &;
		Solver& pop() &;
		Solver& check_sat() &;
		Solver& result() &;
		bool is_unknown() const {
			return _status == UNKNOWN;
		}
		bool is_sat() const {
			return _status == SAT;
		}
		bool is_unsat() const {
			return _status == UNSAT;
		}
		PostExp get_value( PostExp const& e ) &;
		PostExp get_value( PreExp const& e ) &;
		static Solver of( Exp const& );
	private:
		PostExp _get_value( PostExp const& e ) &;
		PostExp _get_value_base( PostExp const& e ) &;
		PostExp _get_value( PreExp const& e ) &;
	};
	class Z3 : public Solver {
	public:
		Z3( Logic const& logic, Opt<OStream> && tee = {} ) :
			Solver( std::make_unique<Proc>("z3",std::vector{"z3","-smt2","-in"},std::move(tee)), logic ) {}
	};
	class CVC5 : public Solver {
	public:
		CVC5( Logic const& logic, Opt<OStream> && tee = {} ) :
			Solver( std::make_unique<Proc>("cvc5",std::vector{"cvc5","--incremental","--produce-models"},std::move(tee)), logic ) {}
	};
	static int test();
};

struct Smt::Sort::_Cons {
	Sort first, second;
};
inline Smt::Sort::Sort( Sort const& x, Sort const& y ) : _un(Ref<_Cons>::make(x,y)) {}
inline OptRef<Smt::Sort::_Cons> Smt::Sort::cons() const & {
	return _un.ref<Ref<_Cons>>() >>= [](auto ref)->OptRef<_Cons>{ return {ref}; };
}
inline Smt::Compare order( Smt::PostExp const& x, Smt::PostExp const& y ) {
	if( x == y ) return { true, false };
	return {Smt::ge(x,y),Smt::gt(x,y)};
}
struct Smt::PreExp::App {
	std::string fun;
	std::vector<PreExp> args;
};
struct Smt::PreExp::Let {
	PreExp val;
	Sort sort;
	std::function<PreExp(PostExp const&)> body;
};

inline Smt::Sort operator,( Smt::Sort const& x, Smt::Sort const& y ) {
	return Smt::Sort(x,y);
}

inline Smt::PostExp operator&&( Smt::PostExp const& x, Smt::PostExp const& y ) {
	return x.conj(y);
}
inline Smt::PostExp operator||( Smt::PostExp const& x, Smt::PostExp const& y ) {
	return x.disj(y);
}
inline Smt::PostExp operator+( Smt::PostExp x, Smt::PostExp const& y ) {
	return x += y;
}
inline Smt::PostExp operator,( Smt::PostExp const& x, Smt::PostExp const& y ) {
	return x.cons(y);
}

inline Smt::PreExp operator&&( Smt::PreExp const& x, Smt::PreExp const& y ) {
	return x.conj(y);
}
inline Smt::PreExp operator||( Smt::PreExp const& x, Smt::PreExp const& y ) {
	return x.disj(y);
}
inline Smt::PreExp operator+( Smt::PreExp const& x, Smt::PreExp const& y ) {
	return x.add(y);
}
inline Smt::PreExp operator*( Smt::PreExp const& x, Smt::PreExp const& y ) {
	return x.mul(y);
}
inline Smt::PreExp operator,( Smt::PreExp const& x, Smt::PreExp const& y ) {
	return x.cons(y);
}
inline std::ostream& operator<<( std::ostream& os, Smt::BaseSort const& x ) {
	return os << x.name();
}
std::ostream& operator<<( std::ostream& os, Smt::Sort const& e );
std::ostream& operator<<( std::ostream& os, Smt::Rat const& r );
std::ostream& operator<<( std::ostream& os, Smt::Val const& v );
std::ostream& operator<<( std::ostream& os, Smt::Fun const& f );
inline std::ostream& operator<<( std::ostream& os, Smt::PostExp const& e ) {
	return os << (Term<Smt::Fun>)e;
}
std::ostream& operator<<( std::ostream& os, Smt::PreExp const& e );
inline std::ostream& operator<<( std::ostream& os, Smt::Compare const& c ) {
	return os << '{' << c.ge << ", " << c.gt << '}';
}

inline std::ostream& operator<<( std::ostream& os, Sum<std::string,Smt::PostExp> const& sum ){
	if( auto const& o = sum.ref<std::string>() ) {
		return os << *o;
	}
	return os << *sum.ref<Smt::PostExp>();
}

#endif
