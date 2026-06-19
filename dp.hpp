#ifndef DP_HPP
#define DP_HPP

#include"trs.hpp"

struct Dp {
	Exp first;
	Exp second;
	size_t org;
	Pos rpos;
	Dp( Exp const& f, Exp const& s, size_t o, Pos p ) : first(f), second(s), org(o), rpos(p) {}
	std::ostream& print_content( std::ostream& ) const&;
	Printable print_content() const& {
		return Printable([&]( auto& os )->auto&{ return print_content(os); });
	}
};

struct Dps {
	size_t max_index;
	Trs::Sig extra_sig;
	Map<size_t,Dp> map;
	template<typename... Args>
	void emplace( Args&&... args ) {
		map.emplace( max_index, Dp( std::forward<Args>(args)... ) );
		max_index++;
	}
};

Dps make_dps( Trs::Sig const& sig, Trs::Rules const& rules, size_t max_ind );

void mark_dps( Trs::Sig const& sig, Dps& dps );

inline std::ostream& operator<<( std::ostream& os, Dp const& dp ) {
	return os << '(' << dp.print_content() << ')';
}

std::ostream& operator<<( std::ostream& os, Dps const& dps );

#endif
