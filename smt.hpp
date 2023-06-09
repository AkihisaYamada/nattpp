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
	class Logic {
		friend Smt;
		Logic( char const* str ) : str(str) {}
	public:
		char const* const str;
	};
	static Logic const QF_LIA, QF_LRA, LIA, LRA, QF_NIA, QF_NRA, NIA, NRA;
	static char constexpr
		AND[] = "and", OR[] = "or", NOT[] = "not", ITE[] = "ite",
		ADD[] = "+", MUL[] = "*", EQ[] = "=", GE[] = ">=", GT[] = ">",
		CONS[] = "cons", CAR[] = "car", CDR[] = "cdr",
		LIST[] = "list", NTH[] = "nth";
	class BaseSort {
	public:
		std::string const name;
	private:
		friend Smt;
		BaseSort( char const* name ) : name(name) {}
		BaseSort( std::string const& name ) : name(name) {}
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
	};
	static BaseSort const BOOL, INT, REAL;
	class PostExp {
		friend Smt;
		typedef std::pair<std::string,std::vector<PostExp>> App;
		Sum<int,Mem<App>> _un;
		explicit PostExp( std::string&& fun, std::vector<PostExp>&& args ) :
			_un(Mem<App>(std::move(fun),std::move(args)) ) {}
		explicit PostExp( std::string const& fun, std::vector<PostExp>&& args ) :
			_un(Mem<App>(fun,std::move(args)) ) {}
		explicit PostExp( std::string&& fun ) : PostExp(std::move(fun),{}) {}
		explicit PostExp( std::string_view const& fun ) : _un(Mem<App>(fun,std::vector<PostExp>())) {}
		explicit PostExp( char const* fun ) : _un(Mem<App>(fun,std::vector<PostExp>())) {}
	public:
		PostExp() : _un(std::in_place_type<int>) {}
		PostExp( PostExp const& other ) : _un(other._un) {}
		PostExp( PostExp && other ) : _un(std::move(other._un)) {}
		PostExp( int n ) : _un(n) {}
		Opt<int const&> num() const & { return _un.ref<int>(); }
		Opt<int&> num() & { return _un.ref<int>(); }
		Opt<App> app() && = delete;
		OptMem<App> app() const & {
			if( auto ref = _un.ref<Mem<App>>() ) {
				return *ref;
			}
			return {};
		}
		OptMem<App> app() & { return OptMem<App>(_un.ref<Mem<App>>()); }
		bool operator==( PostExp const& other ) const {
			return _un == other._un;
		}
		PostExp& operator=( PostExp const& other ) & {
			_un = other._un;
			return *this;
		}
		PostExp& operator=( PostExp&& other ) & {
			_un = std::move(other._un);
			return *this;
		}
		PostExp conj( PostExp const& y ) const;
		PostExp& conj_eq( PostExp const& y ) &;
		PostExp disj( PostExp const& y ) const;
		PostExp& disj_eq( PostExp const& y ) &;
		PostExp operator!() const;
		PostExp add( PostExp const& y ) const;
		PostExp& operator+=( PostExp const& y ) &;
		PostExp mul( PostExp const& y ) const;
		PostExp& operator*=( PostExp const& y ) &;
		PostExp cons( PostExp const& y ) const {
			return PostExp(CONS,{*this,y});
		}
		Exp exp() const;
	};
	static PostExp const TRUE, FALSE;
	static PostExp car( PostExp const& arg );
	static PostExp cdr( PostExp const& arg );
	class PreExp {
		friend Smt;
		using App = std::pair<std::string,std::vector<PreExp>>;
		using Let = std::tuple<PreExp,Sort,std::function<PreExp(PostExp const&)>>;
		using Lazy = std::function<PreExp()>;
		Sum<PostExp,Mem<App>,Mem<Let>,Lazy> _un;
		explicit PreExp( Sort const& sort, PreExp const& val, std::function<PreExp(PostExp const&)> body ) :
			_un( std::in_place_type<Mem<Let>>, val, sort, body ) {}
		explicit PreExp( std::string_view const& fun, std::vector<PreExp>&& args ) :
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
		Proc& _proc;
		Reader _reader;
		size_t _var_count;
		Solver( Proc& proc, Logic const& logic );
		Solver( Solver const& other ) = delete;
		Solver& operator=( Solver const& other ) = delete;
		std::string _make_fresh() &;
	public:
		PostExp declare_const( std::string_view const& name, BaseSort const& sort ) &;
		PostExp declare_fresh( BaseSort const& sort ) {
			std::string ret = _make_fresh();
			return declare_const(ret,sort);
		}
		PostExp expand( PreExp const& p );
		PostExp define_fun(
			std::string_view const& name,
			std::initializer_list<std::pair<std::string_view,std::string_view>> const& params,
			BaseSort const& sort,
			PostExp const& body
		) &;
		template<class T>
			requires (!std::is_convertible_v<T,PostExp> && std::is_convertible_v<T,PreExp>)
		PostExp define_fun(
			std::string_view const& name,
			std::initializer_list<std::pair<std::string_view,std::string_view>> const& params,
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
		bool is_sat() const {
			return _status == SAT;
		}
		bool is_unsat() const {
			return _status == UNSAT;
		}
		PostExp get_value( PostExp const& e ) &;
	};
	class Z3 : private Proc, public Solver {
	public:
		Z3( Logic const& logic, Opt<std::ostream&> tee = {} ) : Proc("z3",{"z3","-smt2","-in"},tee), Solver((Proc&)*this,logic) {}
	};
	static int test();
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
inline Smt::PostExp operator+( Smt::PostExp const& x, Smt::PostExp const& y ) {
	return x.add(y);
}
inline Smt::PostExp operator*( Smt::PostExp const& x, Smt::PostExp const& y ) {
	return x.mul(y);
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
	return os << x.name;
}
std::ostream& operator<<( std::ostream& os, Smt::Sort const& e );
std::ostream& operator<<( std::ostream& os, Smt::PostExp const& e );
std::ostream& operator<<( std::ostream& os, Smt::PreExp const& e );

#endif
