#ifndef _SMT_HPP
#define _SMT_HPP

#include"ref.hpp"
#include"sum.hpp"
#include"proc.hpp"

class Smt {
public:
	static constexpr char TRUE[] = "true";
	static constexpr char FALSE[] = "false";
	static constexpr char ZERO[] = "0";
	static constexpr char ONE[] = "1";
	class Exp {
		struct _BinOp : Mem<std::pair<Exp,Exp>> {
			_BinOp( Exp const& l, Exp const r ) : Mem(Mem<std::pair<Exp,Exp>>::make(l,r)) {}
		};
		struct _UnOp : Mem<Exp> {
			_UnOp( Exp const& a ) : Mem(Mem<Exp>::make(a)) {}
			_UnOp( Exp && a ) : Mem(Mem<Exp>::make(std::move(a))) {}
		};
		struct _And : _BinOp {};
		struct _Or : _BinOp {};
		struct _Not : _UnOp {};
		struct _Add : _BinOp {};
		struct _Mul : _BinOp {};
			typedef Sum<std::string,_And,_Or,_Not,_Add,_Mul> _Un;
		_Un _un;
		Exp( _And const& v ) : _un(v) {}
		Exp( _Or const& v ) : _un(v) {}
		Exp( _Not const& v ) : _un(v) {}
		Exp( _Add const& v ) : _un(v) {}
		Exp( _Mul const& v ) : _un(v) {}
	public:
		explicit Exp( std::string const& v ) : _un(v) {}
		Exp( Exp const& v ) = default;
		Exp operator&&( Exp const& other ) const;
		Exp operator||( Exp const& other ) const;
		Exp operator!() const;
		Exp operator==( Exp const& other ) const;
		Exp operator<=( Exp const& other ) const;
		Exp operator+( Exp const& other ) const;
		Exp operator*( Exp const& other ) const;
		Opt<std::string> sym() && {
			return std::move(_un).ref<std::string>();
		}
		Opt<std::string const&> sym() const & {
			return _un.ref<std::string>();
		}
		auto conj() && {
			return OptMem<std::pair<Exp,Exp>>(std::move(_un).ref<_And>());
		}
		auto conj() const& {
			return OptMem<std::pair<Exp,Exp>>(_un.ref<_And>());
		}
		auto disj() && {
			return OptMem<std::pair<Exp,Exp>>(std::move(_un).ref<_Or>());
		}
		auto disj() const& {
			return OptMem<std::pair<Exp,Exp>>(_un.ref<_Or>());
		}
		auto neg() && {
			return OptMem<Exp>(std::move(_un).ref<_Not>());
		}
		auto neg() const& {
			return OptMem<Exp>(_un.ref<_Not>());
		}
		auto add() && {
			return OptMem<std::pair<Exp,Exp>>(std::move(_un).ref<_Add>());
		}
		auto add() const& {
			return OptMem<std::pair<Exp,Exp>>(_un.ref<_Add>());
		}
		auto mul() && {
			return OptMem<std::pair<Exp,Exp>>(std::move(_un).ref<_Mul>());
		}
		auto mul() const& {
			return OptMem<std::pair<Exp,Exp>>(_un.ref<_Mul>());
		}
		friend std::ostream& operator<<( std::ostream& os, Exp const& e );
		friend bool operator==( Exp const& l, Exp const& r );
		friend bool operator==( Exp const& l, std::string_view const& r );
	};
	class Solver {
		Proc _proc;
		
	};
	static int test();
};

inline bool operator==( Smt::Exp const& l, Smt::Exp const& r ) {
	return l._un == r._un;
}
inline bool operator==( Smt::Exp const& l, std::string_view const& r ) {
	if( auto sym = l.sym() ) {
		return (std::string_view)*sym == r;
	}
	return false;
}

#endif
