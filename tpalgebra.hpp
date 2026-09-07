#ifndef TPALGEBRA_HPP
#define TPALGEBRA_HPP
#include"template.hpp"

template<typename T>
struct TupleVal {
	std::vector<T> vec;
	TupleVal( T&& org ) {
		vec.emplace_back(std::move(org));
	}
	TupleVal( std::vector<T>&& org ) : vec(std::move(org)) {}

	void memoize( Smt::Solver& solver ) & {
		for( auto& coord : vec ) {
			coord.memoize(solver);
		}
	}

	friend Smt::Compare order( TupleVal const& l, TupleVal const& r, Smt::Solver& solver ) {
		size_t n = l.vec.size();
		if( n == 0 ) throw Error("\"tuple algebra: empty tuple\"");
		if( n != r.vec.size() ) throw Error("\"tuple algebra: comparing wrong size\"");
		auto const& ord1 = order(l.vec[0],r.vec[0],solver);
		auto ge2 = solver.let( Smt::BOOL,
			Smt::PreExp::disj( 1, n, [&]( size_t i ){ return l.vec[i].ge(r.vec[i]); })
		);
		return { ord1.ge && ge2, ord1.gt && ge2 };
	}
};

template<typename S, typename T>
Algebra<Template::Sym,TupleVal<T>> tuple_algebra( std::vector<S>&& sorts ) {
	return [sorts=std::move(sorts)]( Template::Sym const& f, std::vector<TupleVal<T>>&& args )->TupleVal<T>{
		if( auto const& smt = f.is_smt() ) {
			return {T(*smt)};
		}
		if( auto const& sym = f.is_fun() ) {
			if( int i = sym->name == "prj0" ? 0 : sym->name == "prj1" ? 1 : sym->name == "prj2" ? 2 : -1; i >= 0 ) {
				if( args.size() != 1 ) throw Error("\"tuple algebra: wrong arity\"");
				auto&& arg = args[0];
				if( arg.vec.size() <= i ) throw Error("\"tuple algebra: bad projection index\"");
				return {T(arg.vec[i])};
			}
			auto vec = std::vector<T>();
			for( auto&& arg : args ) {
				for( auto&& val : arg.vec ) {
					vec.push_back(std::move(val));
				}
			}
			if( sym->name == "tp" ) {
				return TupleVal<T>(std::move(vec));
			}
			return TupleVal<T>(T::ALGEBRA(*sym,std::move(vec)));
		}
		if( auto const& var = f.is_var() ) {
			auto vec = std::vector<T>();
			for( int i = 0; i < sorts.size(); i++ ) {
				vec.emplace_back( typename T::Var( *var + "_" + std::to_string(i), sorts[i] ) );
			}
			return TupleVal<T>(std::move(vec));
		}
		assert(false);
	};
}

#endif