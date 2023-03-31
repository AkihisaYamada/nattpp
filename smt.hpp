#ifndef _SMT_HPP
#define _SMT_HPP

#include<iostream>
#include"exp.hpp"
#include"proc.hpp"

class Smt {
public:
	struct Error : std::exception {
		::Exp message;
		Error( ::Exp const& message ) : message(message) {}
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
	struct Exp : ::Exp {
		using ::Exp::Exp;
		Exp( ::Exp && org ) : ::Exp(std::move(org)) {}
		Exp( ::Exp const& org ) : ::Exp(org) {}
		Exp operator&&( Exp const& other ) const;
		Exp operator||( Exp const& other ) const;
		Exp operator!() const;
		Exp eq( Exp const& other ) const;
		Exp operator>=( Exp const& other ) const;
		Exp operator>( Exp const& other ) const;
		Exp operator+( Exp const& other ) const;
		Exp operator*( Exp const& other ) const;
	};
	static Exp ite( Exp const& x, Exp const& y, Exp const& z );
	class Solver {
		friend Smt;
		enum { UNKNOWN, SOLVING, SAT, UNSAT } _status;
		Proc& _proc;
		::Exp::Reader _reader;
		Solver( Proc& proc ) : _status(UNKNOWN), _proc(proc), _reader(proc.from) {}
		Solver( Solver const& other ) = delete;
		Solver& operator=( Solver const& other ) = delete;
	public:
		void set_logic( std::string_view const& view ) &;
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
		Exp declare_const( std::string_view const& name, std::string_view const& sort ) &;
	};
private:
	public:
	class Z3 : private Proc, public Solver {
	public:
		Z3() : Proc("z3",{"z3","-smt2","-in"}), Solver((Proc&)*this) {
		}
	};
	static int test();
};

#endif
