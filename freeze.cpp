#include<ranges>
#include"algebra.hpp"
#include"problem.hpp"


struct FreezeType {
	std::vector<bool> args;
	bool ret;
	// initially, all positions are freezable, not locked
	FreezeType( uint16_t arity ) : args(arity,true), ret(true) {}
};

bool Problem::freeze() & {
	// three step freezing
	Map<std::string,FreezeType> freeze_info;
	for( auto const& [f,finfo] : main.sig ) {
		freeze_info.emplace(f,FreezeType(finfo.arity));
	}
	// step 1: enumerate freezable positions
	Algebra<std::string,bool> pre = [&]( std::string const& f, std::vector<bool>&& args ){
		if( auto finfo = freeze_info.find(f) ) {
			auto& [fargs,fret] = *finfo;
			size_t n = fargs.size();
			assert( args.size() == n );
			bool pot = false;
			for( uint16_t i = 0; i < n; i++ ) {
				if( args[i] ) {// argument is frozen or constructor
					if( fargs[i] ) {
						pot = true;// potential to freeze
						continue;
					}
				}
				fargs[i] = false;
			}
			fret = pot;
			return fret ||// frozen defined symbol can be frozen further, or
				ASSERTED(main.sig.find(f))->defined_by.empty();// constructor can be frozen
		}
		return false;// variable cannot be frozen
	};
	for( auto const& [i,rule] : main.rules ) {
		auto const& [l,r,w] = rule;
		pre(l);
	}
	// test if any symbol can be frozen
	{
		bool freezable = false;
		for( auto const& [f,finfo] : freeze_info ) {
			if( finfo.ret ) {
				freezable = true;
				main.sig[f].defined_by.clear();// forget raw symbols were defined
			}
		}
		if( !freezable ) return false;// nothing can be frozen
	}
	// step 2: introduce frozen symbols, freeze lhs, and update defined symbols
	std::vector<Trs::Rule> freezers;// freezer rules
	Algebra<std::string,Term<std::string>> lalg =
	[&]( std::string const& f, std::vector<Term<std::string>> const& args ){
		if( auto finfo = freeze_info.find(f) ) {
			auto& [fargs,fret] = *finfo;
			if( fret ) {
				std::string ff = "[" + f;
				std::vector<Term<std::string>> ret_args;
				for( uint16_t i = 0; i < fargs.size(); i++ ) {
					if( fargs[i] ) {// freezing position
						ff.append(1,' ') += args[i].fun();
						ret_args.append_range(args[i].args());
					} else {// raw position
						ret_args.emplace_back(args[i]);
					}
				}
				ff += ']';
				// register the frozen symbol
				auto [it,fresh] = main.sig.emplace(ff,ret_args.size());
				if( fresh ) {// constructing freezing rule f(g(x,...),y,...) -> f*g(x,...,y,...)
					uint16_t fi = 0;
					std::vector<Term<std::string>> largs, frargs;
					for( uint16_t i = 0; i < fargs.size(); i++ ) {
						if( fargs[i] ) {
							std::vector<Term<std::string>> lsubargs;
							for( auto const& subarg : args[i].args() ) {
								auto v = "_"+std::to_string(fi);
								fi++;
								lsubargs.emplace_back(v);
								frargs.emplace_back(v);
							}
							largs.emplace_back(app(args[i].fun(),std::move(lsubargs)));
						} else {
							auto v = "_"+std::to_string(fi);
							fi++;
							largs.emplace_back(v);
							frargs.emplace_back(v);
						}
					}
					freezers.emplace_back(Trs::Rule(app(f,std::move(largs)),app(ff,std::move(frargs))));
				}
				return app(std::move(ff),std::move(ret_args));
			}
		}
		return app(f,std::move(args));
	};
	for( auto& [i,rule] : main.rules ) {
		auto& [l,r,w] = rule;
		l = lalg(l);
		ASSERTED(main.sig.find(l.fun()))->defined_by.emplace(i);
	}

	// step 3: freeze rhss
	Algebra<std::string,Term<std::string>> ralg = [&]( std::string const& f, std::vector<Term<std::string>> const& args ){
		if( auto finfo = freeze_info.find(f) ) {
			auto& [fargs,fret] = *finfo;
			if( fret ) {
				std::string ff = "[" + f;
				std::vector<Term<std::string>> ret_args;
				bool freezable = true;// check if all freezing positions are supplied constructor
				for( uint16_t i = 0; i < fargs.size(); i++ ) {
					if( fargs[i] ) {
						auto const& g = args[i].fun();
						if( main.sig.find(g) && []( auto const& grank ){ return grank.defined_by.empty(); } ) {
							ff.append(1,' ') += g;
							ret_args.append_range(args[i].args());
							continue;
						}
						// freezing position but not freezable
						freezable = false;
						break;
					} else {
						ret_args.emplace_back(std::move(args[i]));
					}
				}
				if( freezable ) {
					ff += ']';
					// don't freeze if the frozen symbol doesn't appears in lhss
					if( main.sig.find(ff) ) {
						return app(std::move(ff),std::move(ret_args));
					}
				}
			}
		}
		return app(f,std::move(args));
	};
	for( auto& [i,rule] : main.rules ) {
		auto& [l,r,w] = rule;
		r = ralg(r);
	}
	// move freezer rules into main rules
	for( auto& rule : freezers ) {
		ASSERTED(main.sig.find(rule.first.fun()))->defined_by.emplace(next_rule);
		insert_rule(main.rules,std::move(rule));
	}
	return true;
}
