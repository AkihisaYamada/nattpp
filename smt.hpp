#ifndef _SMT_HPP
#define _SMT_HPP

#include<iostream>
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
	};
	class Sort {
		using _Cons = std::pair<Sort,Sort>;
		Sum<BaseSort,Mem<_Cons>> _un;
	public:
		Sort( BaseSort const& base ) : _un(base) {}
		Sort( Sort const& x, Sort const& y ) : _un(Mem<_Cons>(x,y)) {}
		Opt<BaseSort const&> base() const & {
			return _un.ref<BaseSort>();
		}
		OptMem<_Cons> cons() const & {
			return OptMem<_Cons>(_un.ref<Mem<_Cons>>());
		}
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
	using Fun = Sum<int,std::string>;
	static Fun const AND, OR, NOT, IMP, ITE, ADD, MUL, EQ, GE, GT, CONS, CAR, CDR, LIST, NTH;
	class PostExp {
		friend Smt;
		Term<Fun> _term;
		PostExp( Term<Fun> const& term ) : _term(term) {}
	public:
		PostExp( PostExp const& ) = default;
		PostExp( PostExp && ) = default;
		PostExp() : _term(std::in_place_type<int>) {}
		PostExp( int i ) : _term(i) {}
		operator Term<Fun> const() const {
			return _term;
		}
		PostExp& operator=( PostExp const& other ) & {
			_term = other;
			return *this;
		}
		PostExp& operator=( PostExp&& other ) & {
			_term = std::move(other);
			return *this;
		}
		bool operator==( PostExp const& other ) const {
			return _term == other._term;
		}
		bool operator!=( PostExp const& other ) const {
			return _term != other._term;
		}
		Opt<int> is_int() && { return _term.fun().ref<int>(); }
		Opt<int const&> is_int() const & { return _term.fun().ref<int>(); }
		int as_int() const& {
			auto opt = is_int();
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
private:
		Opt<int&> is_int() & { return _term.fun().ref<int>(); }
public:
		PostExp& operator&=( PostExp const& y ) &;
		PostExp& operator|=( PostExp const& y ) &;
		PostExp imp( PostExp const& y ) const;
		PostExp operator!() const;
		PostExp& operator+=( PostExp const& y ) &;
		PostExp& mul_eq( PostExp const&, bool linear ) &;
		PostExp cons( PostExp const& y ) const {
			return Term<Fun>(CONS,*this,y);
		}
		Exp exp() const;
	};
	static PostExp const TRUE, FALSE;
	static PostExp car( PostExp const& arg );
	static PostExp cdr( PostExp const& arg );
	struct Compare {
		PostExp ge, gt;
	};
	class PreExp {
		friend Smt;
		using App = std::pair<Fun,std::vector<PreExp>>;
		using Let = std::tuple<PreExp,Sort,std::function<PreExp(PostExp const&)>>;
		using Lazy = std::function<PreExp()>;
		Sum<PostExp,Mem<App>,Mem<Let>,Lazy> _un;
		explicit PreExp( Sort const& sort, PreExp const& val, std::function<PreExp(PostExp const&)> body ) :
			_un( std::in_place_type<Mem<Let>>, val, sort, body ) {}
		explicit PreExp( Fun const& fun, std::vector<PreExp>&& args ) :
			_un(std::in_place_type<Mem<App>>,fun,std::move(args)) {}
	public:
		PreExp( PostExp const& e ) : _un(e) {}
		template<typename T>
			requires std::is_constructible_v<Lazy,T>
		PreExp( T const& lazy ) : _un(std::in_place_type<Lazy>,lazy) {}
		PreExp operator!() const {
			return PreExp(NOT,{*this});
		}
		auto post() && {
			return std::move(_un).ref<PostExp>();
		}
		auto post() const& {
			return _un.ref<PostExp>();
		}
		auto app() && {
			return OptMem<App>(std::move(_un).ref<Mem<App>>());
		}
		auto app() const& {
			return OptMem<App>(_un.ref<Mem<App>>());
		}
		auto let() && {
			return OptMem<Let>(std::move(_un).ref<Mem<Let>>());
		}
		auto let() const& {
			return OptMem<Let>(_un.ref<Mem<Let>>());
		}
		auto lazy() && {
			return Opt<Lazy>(std::move(_un).ref<Lazy>());
		}
		auto lazy() const& {
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
		PreExp& operator+=( PreExp const& y ) & {
			return *this = add(y);
		}
		PreExp mul( PreExp const& y ) const {
			return PreExp(MUL,{*this,y});
		}
		PreExp& operator*=( PreExp const& y ) & {
			return *this = mul(y);
		}
		PreExp cons( PreExp const& y ) const {
			return PreExp(CONS,{*this,y});
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
	static Algebra::Intp<std::string,Smt::PreExp> const ALGEBRA;
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
		Solver& operator=( Solver&& other ) = default;
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
		template<class T>
			requires (!std::is_convertible_v<T,PostExp> && std::is_convertible_v<T,PreExp>)
		PostExp let( Sort const& sort, T const& val ) & {
			return let(sort,expand(val));
		}
		Solver& ass( PostExp const& e ) &;
		template<class T>
			requires (!std::is_convertible_v<T,PostExp> && std::is_convertible_v<T,PreExp>)
		Solver& ass( T const& p ) & {
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
		static Solver of( Exp const& );
	};
	class Z3 : public Solver {
	public:
		Z3( Logic const& logic, Opt<OStream> && tee = {} ) :
			Solver( std::make_unique<Proc>("z3",std::vector{"z3","-smt2","-in"},std::move(tee)), logic ) {}
	};
	static int test();

};

inline Smt::Sort operator,( Smt::Sort const& x, Smt::Sort const& y ) {
	return Smt::Sort(x,y);
}

inline Smt::PostExp operator&&( Smt::PostExp x, Smt::PostExp const& y ) {
	return x &= y;
}
inline Smt::PostExp operator||( Smt::PostExp x, Smt::PostExp const& y ) {
	return x |= y;
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
std::ostream& operator<<( std::ostream& os, Smt::Fun const& f );
inline std::ostream& operator<<( std::ostream& os, Smt::PostExp const& e ) {
	return os << (Term<Smt::Fun>)e;
}
std::ostream& operator<<( std::ostream& os, Smt::PreExp const& e );


#endif
