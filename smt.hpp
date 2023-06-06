#ifndef _SMT_HPP
#define _SMT_HPP

#include<iostream>
#include"algebra.hpp"
#include"proc.hpp"

class Smt {
public:
	struct Error : ::Exp::Error {
		using ::Exp::Error::Error;
	};
	class Logic {
		friend Smt;
		Logic( char const* str ) : str(str) {}
	public:
		char const* const str;
	};
	static const Logic QF_LIA, QF_LRA, LIA, LRA, QF_IA, QF_RA, IA, RA;
	static constexpr char AND[] = "and";
	static constexpr char OR[] = "or";
	static constexpr char NOT[] = "not";
	static constexpr char ADD[] = "+";
	static constexpr char MUL[] = "*";
	static constexpr char EQ[] = "=";
	static constexpr char GE[] = ">=";
	static constexpr char GT[] = ">";
	static constexpr char ITE[] = "ite";
	static constexpr char CONS[] = "cons";
	static constexpr char CAR[] = "car";
	static constexpr char CDR[] = "cdr";
	static constexpr char LIST[] = "list";
	static constexpr char NTH[] = "nth";
	class Sort;
	class BaseSort {
	public:
		std::string const name;
		Sort operator,( Sort const& rest ) const;
	private:
		friend Smt;
		BaseSort( char const* name ) : name(name) {}
		BaseSort( std::string const& name ) : name(name) {}
	};
	class Sort : public ExpView {
		friend Smt;
		Exp _exp;
		Sort( Exp const& exp ) : _exp(exp) {}
	public:
		Sort( BaseSort const& base ) : _exp(base.name) {}
		Sort operator,( Sort const& rest ) const {
			return Sort({CONS,_exp,rest._exp});
		}
		Opt<BaseSort> base() const & {
			auto const& fun = _exp.fun();
			if( fun == CONS ) {
				return {};
			}
			return BaseSort(fun);
		}
		Exp const& view_exp() const override { return _exp; }
	};
	static BaseSort const BOOL, INT, REAL;
	class PostExp : private Exp {
		friend Smt;
		using Exp::Exp;
		PostExp( Exp&& x ) : Exp(std::move(x)) {}
		PostExp( Exp const& x ) : Exp(x) {}
	public:
		PostExp() {}
		PostExp( PostExp const& other ) : Exp(other) {}
		PostExp( PostExp && other ) : Exp(std::move(other)) {}
		PostExp( int n ) : Exp(n) {}
		Smt::PostExp operator&&( Smt::PostExp const& y ) const;
		Smt::PostExp operator||( Smt::PostExp const& y ) const;
		PostExp& disj_eq( PostExp const& arg ) &;
		PostExp& conj_eq( PostExp const& arg ) &;
		bool operator==( PostExp const& other ) const {
			return Exp::operator==(other);
		}
		PostExp& operator=( PostExp const& other ) & {
			Exp::operator=(other);
			return *this;
		}
		PostExp& operator=( PostExp&& other ) & {
			Exp::operator=(std::move(other));
			return *this;
		}
		PostExp operator!() const;
		PostExp operator+( PostExp const& arg ) const;
		PostExp& operator+=( PostExp const& other ) &;
		PostExp operator*( PostExp const& arg ) const;
		PostExp& operator*=( PostExp const& other ) &;
		PostExp operator,( PostExp const& other ) const {
			return Exp(CONS,(Exp)*this,(Exp)other);
		}
		Exp const exp() const {
			return *this;
		}
	};
	static PostExp const TRUE, FALSE, ZERO, ONE;
	static PostExp car( PostExp const& arg );
	static PostExp cdr( PostExp const& arg );
	class PreExp {
		friend Smt;
		using App = std::pair<std::string,std::vector<PreExp>>;
		using Let = std::tuple<PreExp,Sort,std::function<PreExp(PreExp const&)>>;
		using Lazy = std::function<PreExp()>;
		Sum<PostExp,Mem<App>,Mem<Let>,Lazy> _un;
		explicit PreExp( Sort const& sort, PreExp const& val, std::function<PreExp(PreExp const&)> body ) :
			_un( std::in_place_type<Mem<Let>>, val, sort, body ) {}
		explicit PreExp( std::string_view const& fun, std::vector<PreExp>&& args ) :
			_un(std::in_place_type<Mem<App>>,fun,std::move(args)) {}
	public:
		PreExp( const char* str ) : _un(std::in_place_type<PostExp>,str) {}
		PreExp( std::string && str ) : _un(std::in_place_type<PostExp>,std::move(str)) {}
		PreExp( std::string_view && str ) : _un(std::in_place_type<PostExp>,std::move(str)) {}
		PreExp( int n ) : _un(std::in_place_type<PostExp>,std::to_string(n)) {}
		PreExp( PostExp const& e ) : _un(e) {}
		template<typename T> requires std::is_convertible_v<T,Lazy>
		PreExp( T const& lazy ) : _un(lazy) {}
		PreExp operator&&( PreExp const& y ) const {
			return PreExp(AND,{*this,y});
		}
		PreExp operator||( PreExp const& y ) const {
			return PreExp(OR,{*this,y});
		}
		PreExp operator!() const {
			return PreExp(NOT,{*this});
		}
		PreExp operator+( PreExp const& y ) const {
			return PreExp(ADD,{*this,y});
		}
		PreExp operator*( PreExp const& y ) const {
			return PreExp(MUL,{*this,y});
		}
		PreExp& operator+=( PreExp const& y ) & {
			return *this = *this + y;
		}
		PreExp& operator*=( PreExp const& y ) & {
			return *this = *this * y;
		}
		PreExp operator,( PreExp const& y ) const {
			return PreExp(CONS,{*this,y});
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
	};
	static PreExp car( PreExp const& arg ) {
		return PreExp{CAR,{arg}};
	}
	static PreExp cdr( PreExp const& arg ) {
		return PreExp{CDR,{arg}};
	}
	static PostExp eq( PostExp const& x, PostExp const& y ) {
		return PostExp{EQ,(Exp)x,(Exp)y};
	}
	template<class T>
		requires (!std::is_convertible_v<T,PostExp> && std::is_convertible_v<T,PreExp>)
	static PreExp eq( T const& x, PreExp const& y ) {
		return PreExp(EQ,{x,y});
	}
	template<class T>
		requires (!std::is_convertible_v<T,PostExp> && std::is_convertible_v<T,PreExp>)
	static PreExp eq( PreExp const& x, T const& y ) {
		return PreExp(EQ,{x,y});
	}
	static PostExp ge( PostExp const& x, PostExp const& y ) {
		return PostExp{GE,(Exp)x,(Exp)y};
	}
	template<class T>
		requires (!std::is_convertible_v<T,PostExp> && std::is_convertible_v<T,PreExp>)
	static PreExp ge( T const& x, PreExp const& y ) {
		return PreExp(GE,{x,y});
	}
	template<class T>
		requires (!std::is_convertible_v<T,PostExp> && std::is_convertible_v<T,PreExp>)
	static PreExp ge( PreExp const& x, T const& y ) {
		return PreExp(GE,{x,y});
	}
	static PostExp gt( PostExp const& x, PostExp const& y ) {
		return PostExp{GT,(Exp)x,(Exp)y};
	}
	template<class T>
		requires (!std::is_convertible_v<T,PostExp> && std::is_convertible_v<T,PreExp>)
	static PreExp gt( T const& x, PreExp const& y ) {
		return PreExp(GT,{x,y});
	}
	template<class T>
		requires (!std::is_convertible_v<T,PostExp> && std::is_convertible_v<T,PreExp>)
	static PreExp gt( PreExp const& x, T const& y ) {
		return PreExp(GT,{x,y});
	}
private:
	struct _PostIf2 {
		PostExp const& i;
		PostExp const& t;
		PostExp operator^( PostExp const& e ) const {
			return PostExp(ITE,(Exp)i,(Exp)t,(Exp)e);
		}
		PreExp operator^( PreExp const& e ) const {
			return PreExp(ITE,{i,t,e});
		}
	};
	struct _PreIf2 {
		PreExp const& i;
		PreExp const& t;
		PreExp operator^( PreExp const& e ) const {
			return PreExp(ITE,{i,t,e});
		}
	};
	struct _PostIf1 {
		PostExp const& i;
		_PostIf2 operator^( PostExp const& t ) const {
			return {i,t};
		}
		_PreIf2 operator^( PreExp const& t ) const {
			return {i,t};
		}
	};
	struct _PreIf1 {
		PreExp const& i;
		_PreIf2 operator^( PreExp const& t ) const {
			return {i,t};
		}
	};
public:
	static _PostIf1 If( PostExp const& i ) {
		return {i};
	}
	template<class T> requires (!std::is_convertible_v<T,PostExp>)
	static _PreIf1 If( T const& i ) {
		return {i};
	}
	class Let {
		Sort const& sort;
		PreExp const& val;
		Let( Let const& ) = delete;
	public:
		explicit Let( Sort const& sort, PreExp const& val ) : sort(sort), val(val) {}
		PreExp operator^( std::function<PreExp(PreExp const&)> body ) const {
			return PreExp(sort,val,body);
		}
	};
	static Algebra::Intp<Smt::PreExp> const ALGEBRA;
	class Solver {
		friend Smt;
		enum { UNKNOWN, SOLVING, SAT, UNSAT } _status;
		Proc& _proc;
		::Exp::Reader _reader;
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

inline Smt::Sort Smt::BaseSort::operator,( Sort const& rest ) const {
	return (Sort(*this), rest);
}
template<class T>
	requires (!std::is_convertible_v<T,Smt::PostExp> && std::is_convertible_v<T,Smt::PreExp>)
inline Smt::PreExp operator+( Smt::PostExp const& x, T const& y ) {
	return Smt::PreExp(x) + y;
}
template<class T>
	requires (!std::is_convertible_v<T,Smt::PostExp> && std::is_convertible_v<T,Smt::PreExp>)
inline Smt::PreExp operator*( Smt::PostExp const& x, T const& y ) {
	return Smt::PreExp(x) * y;
}
template<class T>
	requires (!std::is_convertible_v<T,Smt::PostExp> && std::is_convertible_v<T,Smt::PreExp>)
inline Smt::PreExp operator&&( Smt::PostExp const& x, T const& y ) {
	return Smt::PreExp(x) && y;
}
template<class T>
	requires (!std::is_convertible_v<T,Smt::PostExp> && std::is_convertible_v<T,Smt::PreExp>)
inline Smt::PreExp operator||( Smt::PostExp const& x, T const& y ) {
	return Smt::PreExp(x) || y;
}
inline std::ostream& operator<<( std::ostream& os, Smt::PostExp const& e ) {
	return os << e.exp();
}
std::ostream& operator<<( std::ostream& os, Smt::PreExp const& e );

#endif
