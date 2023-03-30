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
		friend Smt;
		struct _BinOp : Mem<std::pair<Exp,Exp>> {
			_BinOp( Exp const& l, Exp const r ) : Mem(Mem<std::pair<Exp,Exp>>::make(l,r)) {}
		};
		struct _UnOp : Mem<Exp> {
			_UnOp( Exp const& a ) : Mem(Mem<Exp>::make(a)) {}
			_UnOp( Exp && a ) : Mem(Mem<Exp>::make(std::move(a))) {}
		};
		struct _TrOp : Mem<std::tuple<Exp,Exp,Exp>> {
			_TrOp( Exp const& x, Exp const& y, Exp const& z ) :
				Mem(Mem<std::tuple<Exp,Exp,Exp>>::make(x,y,z)) {}
		};
		struct _And : _BinOp {};
		struct _Or : _BinOp {};
		struct _Not : _UnOp {};
		struct _Ite : _TrOp {};
		struct _Eq : _BinOp {};
		struct _Ge : _BinOp {};
		struct _Gt : _BinOp {};
		struct _Add : _BinOp {};
		struct _Mul : _BinOp {};
		typedef Sum<std::string,_And,_Or,_Not,_Ite,_Eq,_Ge,_Gt,_Add,_Mul> _Un;
		_Un _un;
		template<typename T> requires std::is_convertible_v<T,_Un> && (!std::is_convertible_v<T,std::string>)
		Exp( T const& v ) : _un(v) {}
	public:
		template<typename T> requires std::is_convertible_v<T,std::string_view>
		Exp( T const& v ) : _un(v) {}
		Exp( Exp const& v ) = default;
		Exp operator&&( Exp const& other ) const;
		Exp operator||( Exp const& other ) const;
		Exp operator!() const;
		Exp eq( Exp const& other ) const;
		Exp operator>=( Exp const& other ) const;
		Exp operator>( Exp const& other ) const;
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
		auto ite() const& {
			return OptMem<std::tuple<Exp,Exp,Exp>>(_un.ref<_Ite>());
		}
		auto eq() const& {
			return OptMem<std::pair<Exp,Exp>>(_un.ref<_Eq>());
		}
		auto ge() const& {
			return OptMem<std::pair<Exp,Exp>>(_un.ref<_Ge>());
		}
		auto gt() const& {
			return OptMem<std::pair<Exp,Exp>>(_un.ref<_Gt>());
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
		bool operator==( Exp const& other ) const {
			return _un == other._un;
		}
		friend std::ostream& operator<<( std::ostream& os, Exp const& e );
	};
	static Exp ite( Exp const& x, Exp const& y, Exp const& z );
	class Solver {
		Proc _proc;
		
	};
	static int test();
};

#endif
