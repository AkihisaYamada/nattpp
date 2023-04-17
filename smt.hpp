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
private:
	struct _LET_tag {};
	struct _LAZY_tag {};
public:
	class Exp {
		friend Smt;
		using App = std::pair<Exp,std::vector<Exp>>;
		using Let = std::tuple<Exp,Sort,std::function<Exp(Exp const&)>>;
		using Lazy = std::function<Exp()>;
		Sum<std::string,Mem<App>,Mem<Let>,Lazy> _un;
		explicit Exp( _LET_tag const&, Exp const& val, std::string_view const& sort, std::function<Exp(Exp const&)> body ) :
			_un( std::in_place_type<Mem<Let>>, val, sort, body ) {}
	public:
		Exp( const char* str ) : _un(std::in_place_type<std::string>,str) {}
		Exp( std::string_view const& str ) : _un(std::in_place_type<std::string>,str) {}
		Exp( int n ) : _un(std::to_string(n)) {}
		Exp( Exp const& fun, std::initializer_list<Exp> const& args ) :
			_un(std::in_place_type<Mem<App>>,fun,args) {}
		Exp( Exp const& fun, std::vector<Exp>&& args ) :
			_un(std::in_place_type<Mem<App>>,fun,std::move(args)) {}
		template<typename T> requires std::is_convertible_v<T,Lazy>
		Exp( T const& lazy ) : _un(lazy) {}
		Exp operator&&( Exp const& y ) const {
			return Exp(AND,{*this,y});
		}
		Exp operator||( Exp const& y ) const {
			return Exp(OR,{*this,y});
		}
		Exp operator!() const {
			return Exp(NOT,{*this});
		}
		Exp eq( Exp const& y ) const {
			return Exp(EQ,{*this,y});
		}
		Exp ge( Exp const& y ) const {
			return Exp(GE,{*this,y});
		}
		Exp gt( Exp const& y ) const {
			return Exp(GT,{*this,y});
		}
		Exp operator+( Exp const& y ) const {
			return Exp(ADD,{*this,y});
		}
		Exp& operator+=( Exp const& y ) & {
			return *this = *this + y;
		}
		Exp operator*( Exp const& y ) const {
			return Exp(MUL,{*this,y});
		}
		Exp& operator*=( Exp const& y ) & {
			return *this = *this * y;
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
			return Opt<Lazy>(std::move(_un).ref<Lazy>());
		}
		auto lazy() const& {
			return _un.ref<Lazy>();
		}
	};
public:
	class If {
		Exp const& i;
		struct _If2 {
			Exp const& i;
			Exp const& t;
			Exp operator^( Exp const& e ) const {
				return Exp(ITE,{i,t,e});
			}
		};
	public:
		explicit If( Exp const& i ) : i(i) {}
		_If2 operator^( Exp const& t ) const {
			return {i,t};
		}
	};
	class Let {
		Exp const& val;
		char const* sort;
	public:
		explicit Let( Exp const& val, char const* sort ) : val(val), sort(sort) {}
		Exp operator^( std::function<Exp(Exp const&)> body ) const {
			return Exp(_LET_tag(),val,sort,body);
		}
	};
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
		Solver& ass( ::Exp const& e ) &;
		::Exp expand( Exp const& p );
		Solver& ass( Exp const& p ) & {
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
		::Exp get_value( ::Exp const& e ) &;
	};
private:
	public:
	class Z3 : private Proc, public Solver {
	public:
		Z3( Opt<std::ostream&> tee = {} ) : Proc("z3",{"z3","-smt2","-in"},tee), Solver((Proc&)*this) {}
	};
	static int test();
};

std::ostream& operator<<( std::ostream& os, Smt::Exp const& e );

#endif
