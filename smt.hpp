#ifndef _SMT_HPP
#define _SMT_HPP

#include"ref.hpp"
#include"sum.hpp"
#include"proc.hpp"

class Smt {
public:
	class Exp {
		struct _Add : Mem<std::pair<Exp,Exp>> {
			_Add( Exp const& l, Exp const& r ) : Mem(Mem::make(l,r)) {}
		};
		struct _Mul : Mem<std::pair<Exp,Exp>> {
			_Mul( Exp const& l, Exp const& r ) : Mem(Mem::make(l,r)) {}
		};
		Sum<std::string,_Add,_Mul> _un;
		Exp( _Add const& v ) : _un(v) {}
		Exp( _Mul const& v ) : _un(v) {}
	public:
		Exp( std::string const& v ) : _un(v) {}
		Exp operator+( Exp const& other ) const;
		Exp operator*( Exp const& other ) const;
		Opt<std::string> sym() && {
			return std::move(_un).ref<std::string>();
		}
		Opt<std::string const&> sym() const & {
			return _un.ref<std::string>();
		}
		OptMem<std::pair<Exp,Exp>> add() && {
			if( auto p = std::move(_un).ref<_Add>() ) {
				return std::move(*p);
			}
			return {};
		}
		OptMem<std::pair<Exp,Exp>> add() const& {
			if( auto p = _un.ref<_Add>() ) {
				return *p;
			}
			return {};
		}
		OptMem<std::pair<Exp,Exp>> mul() && {
			if( auto p = std::move(_un).ref<_Mul>() ) {
				return std::move(*p);
			}
			return {};
		}
		OptMem<std::pair<Exp,Exp>> mul() const& {
			if( auto p = _un.ref<_Mul>() ) {
				return *p;
			}
			return {};
		}
		friend std::ostream& operator<<( std::ostream& os, Exp const& e );
		friend bool operator==( Exp const& l, Exp const& r );
		friend bool operator==( Exp const& l, std::string_view const& r );
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
