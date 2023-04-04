#ifndef _SMT_HPP
#define _SMT_HPP

#include<iostream>
#include"exp.hpp"
#include"proc.hpp"

class Smt {
public:
	struct Error : ::Exp::Error {
		using ::Exp::Error::Error;
	};
	static constexpr char TRUE[] = "true";
	static constexpr char FALSE[] = "false";
	static constexpr char AND[] = "and";
	static constexpr char OR[] = "or";
	static constexpr char NOT[] = "not";
	static constexpr char ADD[] = "+";
	static constexpr char MUL[] = "*";
	static constexpr char EQ[] = "=";
	static constexpr char GE[] = ">=";
	static constexpr char GT[] = ">";
	static constexpr char ITE[] = "ite";
	static constexpr char ZERO[] = "0";
	static constexpr char ONE[] = "1";
	static constexpr char CONS[] = "cons";
	static constexpr char CAR[] = "car";
	static constexpr char CDR[] = "cdr";
	static constexpr char LIST[] = "list";
	static constexpr char NTH[] = "nth";
	using Sort = std::string;
	struct Exp : ::Exp {
		using ::Exp::Exp;
		Exp( ::Exp && org ) : ::Exp(std::move(org)) {}
		Exp( ::Exp const& org ) : ::Exp(org) {}
		Exp operator&&( Exp const& other ) const;
		Exp operator||( Exp const& other ) const;
		Exp operator!() const;
		Exp eq( Exp const& other ) const;
		Exp ge( Exp const& other ) const;
		Exp gt( Exp const& other ) const;
		Exp operator+( Exp const& other ) const;
		Exp operator*( Exp const& other ) const;
	};
	static Exp ite( Exp const& x, Exp const& y, Exp const& z );
private:
	struct _LET_tag {};
	struct _LAZY_tag {};
public:
	class PreExp {
		friend Smt;
		using App = std::vector<PreExp>;
		using Let = std::tuple<PreExp,Sort,std::function<PreExp(PreExp const&)>>;
		using Lazy = std::function<PreExp()>;
		using Lazy2 = std::tuple<char const*,PreExp,Lazy>;
		using Lazy3 = std::tuple<char const*,PreExp,Lazy,Lazy>;
		Sum<std::string,Mem<App>,Mem<Let>,Mem<Lazy2>,Mem<Lazy3>> _un;
		PreExp( _LET_tag const&, PreExp const& val, std::string_view const& sort, std::function<PreExp(PreExp const&)> body ) :
			_un( std::in_place_type<Mem<Let>>, val, sort, body ) {}
		PreExp( _LAZY_tag const&, char const* op, PreExp const& x, std::function<PreExp()> y ) :
			_un(std::in_place_type<Mem<Lazy2>>,op,x,y) {}
		PreExp( _LAZY_tag const&, char const* op, PreExp const& x, std::function<PreExp()> y, std::function<PreExp()> z ) :
			_un(std::in_place_type<Mem<Lazy3>>,op,x,y,z) {}
	public:
		PreExp( const char* str ) : _un(std::in_place_type<std::string>,str) {}
		PreExp( std::string_view const& str ) : _un(std::in_place_type<std::string>,str) {}
		PreExp( std::initializer_list<PreExp> list ) : _un(std::in_place_type<Mem<App>>,list) {}
		PreExp operator&&( std::function<PreExp()> y ) const {
			return PreExp(_LAZY_tag{},AND,*this,y);
		}
		PreExp operator||( std::function<PreExp()> y ) const {
			return PreExp(_LAZY_tag{},OR,*this,y);
		}
		PreExp eq( PreExp const& y ) const {
			return {EQ,*this,y};
		}
		PreExp ge( PreExp const& y ) const {
			return {GE,*this,y};
		}
		PreExp gt( PreExp const& y ) const {
			return {GT,*this,y};
		}
		PreExp operator+( PreExp const& y ) const {
			return {ADD,*this,y};
		}
		PreExp operator*( PreExp const& y ) const {
			return {MUL,*this,y};
		}
		Opt<std::string> sym() && {
			return std::move(_un).ref<std::string>();
		}
		Opt<std::string const&> sym() const& {
			return _un.ref<std::string>();
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
			return OptMem<Lazy2>(std::move(_un).ref<Mem<Lazy2>>());
		}
		auto lazy() const& {
			return OptMem<Lazy2>(_un.ref<Mem<Lazy2>>());
		}
		auto lazy3() && {
			return OptMem<Lazy3>(std::move(_un).ref<Mem<Lazy3>>());
		}
		auto lazy3() const& {
			return OptMem<Lazy3>(_un.ref<Mem<Lazy3>>());
		}
	};
private:
	struct _If2 {
		PreExp const& i;
		std::function<PreExp()> const& t;
		PreExp operator^( std::function<PreExp()> const& e ) const {
			return PreExp(_LAZY_tag{},ITE,i,t,e);
		}
	};
	struct _If1 {
		PreExp const& i;
		_If2 operator^( std::function<PreExp()> const& t ) const {
			return {i,t};
		}
	};
	struct _If {
		_If1 operator^( PreExp const& i ) const {
			return {i};
		}
	};
	struct _Let2 {
		PreExp const& val;
		char const* sort;
		PreExp operator^( std::function<PreExp(PreExp const&)> body ) const {
			return PreExp(_LET_tag(),val,sort,body);
		}
	};
	struct _Let1 {
		PreExp const& val;
		_Let2 operator^( char const* sort ) const {
			return {val,sort};
		}
	};
	struct _Let {
		_Let1 operator^( PreExp const& val ) const {
			return {val};
		}
	};
public:
	static constexpr _If IF = {};
	static constexpr _Let LET = {};
	class Solver {
		friend Smt;
		enum { UNKNOWN, SOLVING, SAT, UNSAT } _status;
		Proc& _proc;
		::Exp::Reader _reader;
		size_t _var_count;
		Solver( Proc& proc ) : _status(UNKNOWN), _proc(proc), _reader(proc.from), _var_count(0) {}
		Solver( Solver const& other ) = delete;
		Solver& operator=( Solver const& other ) = delete;
		std::string _make_fresh() &;
	public:
		void set_logic( std::string_view const& logic ) &;
		Exp declare_const( std::string_view const& name, std::string_view const& sort ) &;
		Exp define_fun(
			std::string_view const& name,
			std::initializer_list<std::pair<std::string_view,std::string_view>> const& params,
			std::string_view const& sort,
			Exp const& body
		) &;
		Solver& ass( Exp const& e ) &;
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
		Exp get_value( Exp const& e ) &;
		Exp expand( PreExp const& p );
		Solver& ass( PreExp const& p ) & {
			return ass(expand(p));
		}
	};
private:
	public:
	class Z3 : private Proc, public Solver {
	public:
		Z3( Opt<std::ostream&> tee = {} ) : Proc("z3",{"z3","-smt2","-in"},tee), Solver((Proc&)*this) {}
	};
	static int test();
};

#endif
