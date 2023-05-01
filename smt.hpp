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
	class PostExp : public ExpView {
		friend Smt;
		Exp _exp;
		PostExp( Exp const& exp ) : _exp(exp) {}
	public:
		PostExp() {}
		PostExp( PostExp const& other ) : _exp(other._exp) {}
		PostExp( PostExp && other ) : _exp(std::move(other._exp)) {}
		PostExp operator *( PostExp const& arg ) const;
		Exp const& view_exp() const override { return _exp; }
		PostExp& disj_eq( PostExp const& arg ) &;
		PostExp& conj_eq( PostExp const& arg ) &;
		bool operator==( PostExp const& other ) const {
			return _exp == other._exp;
		}
		PostExp& operator=( PostExp const& other ) & {
			_exp = other._exp;
			return *this;
		}
		PostExp& operator=( PostExp&& other ) & {
			_exp = std::move(other._exp);
			return *this;
		}
	};
	static PostExp Car( PostExp const& arg );
	static PostExp Cdr( PostExp const& arg );
	class PreExp {
		friend Smt;
		using App = std::pair<std::string,std::vector<PreExp>>;
		using Let = std::tuple<PreExp,Sort,std::function<PreExp(PreExp const&)>>;
		using Lazy = std::function<PreExp()>;
		Sum<Mem<App>,Mem<Let>,Lazy> _un;
		explicit PreExp( Sort const& sort, PreExp const& val, std::function<PreExp(PreExp const&)> body ) :
			_un( std::in_place_type<Mem<Let>>, val, sort, body ) {}
		explicit PreExp( std::string_view const& fun, std::vector<PreExp>&& args ) :
			_un(std::in_place_type<Mem<App>>,fun,std::move(args)) {}
	public:
		PreExp( const char* str ) : _un(Mem<App>(App(str,{}))) {}
		PreExp( std::string const& str ) : _un(Mem<App>(App(str,{}))) {}
		PreExp( std::string_view const& str ) : _un(Mem<App>(App(str,{}))) {}
		PreExp( int n ) : _un(Mem<App>(App(std::to_string(n),{}))) {}
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
		PreExp eq( PreExp const& y ) const {
			return PreExp(EQ,{*this,y});
		}
		PreExp ge( PreExp const& y ) const {
			return PreExp(GE,{*this,y});
		}
		PreExp gt( PreExp const& y ) const {
			return PreExp(GT,{*this,y});
		}
		PreExp operator+( PreExp const& y ) const {
			return PreExp(ADD,{*this,y});
		}
		PreExp& operator+=( PreExp const& y ) & {
			return *this = *this + y;
		}
		PreExp operator*( PreExp const& y ) const {
			return PreExp(MUL,{*this,y});
		}
		PreExp& operator*=( PreExp const& y ) & {
			return *this = *this * y;
		}
		PreExp operator,( PreExp const& y ) const {
			return PreExp(CONS,{*this,y});
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
	struct Const : PreExp, PostExp {
		Const( char const* name ) : PreExp(name), PostExp(name) {}
		Const( std::string_view const& name ) : PreExp(name), PostExp(name) {}
	};
	static Const const TRUE, FALSE, ZERO, ONE;
	static PreExp Car( PreExp const& arg ) {
		return PreExp{CAR,{arg}};
	}
	static PreExp Cdr( PreExp const& arg ) {
		return PreExp{CDR,{arg}};
	}
	class If {
		PreExp const& i;
		struct _If2 {
			PreExp const& i;
			PreExp const& t;
			PreExp operator^( PreExp const& e ) const {
				return PreExp(ITE,{i,t,e});
			}
		};
	public:
		explicit If( PreExp const& i ) : i(i) {}
		_If2 operator^( PreExp const& t ) const {
			return {i,t};
		}
	};
	class Let {
		Sort const& sort;
		Let( Let const& ) = delete;
		struct Let2 {
			Sort const& sort;
			PreExp const& val;
			PreExp operator^( std::function<PreExp(PreExp const&)> body ) const {
				return PreExp(sort,val,body);
			}
		};
	public:
		explicit Let( Sort const& sort ) : sort(sort) {}
		Let2 operator^( PreExp const& val ) const {
			return Let2{sort,val};
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
		PreExp _expand_let( Sort const& sort, PostExp const& val );
	public:
		Const declare_const( std::string_view const& name, BaseSort const& sort ) &;
		std::string declare_fresh( BaseSort const& sort ) {
			std::string ret = _make_fresh();
			declare_const(ret,sort);
			return ret;
		}
		PostExp expand( PreExp const& p );
		Const define_fun(
			std::string_view const& name,
			std::initializer_list<std::pair<std::string_view,std::string_view>> const& params,
			BaseSort const& sort,
			PostExp const& body
		) &;
		Const define_fun(
			std::string_view const& name,
			std::initializer_list<std::pair<std::string_view,std::string_view>> const& params,
			BaseSort const& sort,
			PreExp const& body
		) & {
			return define_fun(name,params,sort,expand(body));
		}
		Solver& ass( PostExp const& e ) &;
		Solver& ass( PreExp const& p ) & {
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
inline std::ostream& operator<<( std::ostream& os, Smt::Const const& e ) {
	return os << (Smt::PostExp)e;
}
std::ostream& operator<<( std::ostream& os, Smt::PreExp const& e );

#endif
