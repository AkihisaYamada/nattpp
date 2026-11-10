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

	Smt::Compare order( TupleVal const& r, Smt::Solver& solver ) const {
		size_t n = vec.size();
		if( n == 0 ) throw Error("#tuple_algebra","\"comparing empty tuple\"");
		if( n != r.vec.size() ) throw Error("#tuple_algebra","\"comparing wrong size\"");
		auto const& ord0 = vec[0].order(r.vec[0],solver);
		auto ge2 = Smt::PostExp::conj( 1, n, [&]( size_t i ){ return vec[i].ge(r.vec[i]); });
		return { ord0.ge && ge2, ord0.str };
	}
	Smt::Compare co_order( TupleVal const& r, Smt::Solver& solver ) const {
		assert(false);
	}
	Smt::PostExp separate( TupleVal const& r ) const {
		size_t n = vec.size();
		if( n == 0 ) throw Error("#tuple_algebra","\"separating empty tuple\"");
		if( n != r.vec.size() ) throw Error("#tuple_algebra","\"separating wrong size\"");
		return Smt::PostExp::disj( 0, n, [&]( size_t i ){ return vec[i].separate(r.vec[i]); });
	}
	Smt::PostExp gt_if( TupleVal const& r, TupleVal const& t, Smt::Solver& solver ) const& {
		throw Error("#tpalgebra","\"gt_if for tuple\"");
	}
	friend std::ostream& operator<<( std::ostream& os, TupleVal const& t ) {
		os << "(tp";
		for( auto const& val : t.vec ) {
			os << ' ' << val;
		}
		return os << ')';
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
				if( args.size() != 1 ) throw Error("#tuple-algebra","\"too many arguments\"",sym->name,std::to_string(args.size()));
				auto&& arg = args[0];
				if( arg.vec.size() <= i ) throw Error("#tuple-algebra","\"bad projection index\"");
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