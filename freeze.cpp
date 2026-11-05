#include<ranges>
#include"algebra.hpp"
#include"problem.hpp"


struct FreezeType {
	bool freezer;// this symbol can freeze an argument
	bool freezee;// this symbol can be frozen into the parent
	bool visited;
	std::vector<std::pair<bool,bool>> args;
	static FreezeType uncurry( Trs::Rank const& rank ) {
		FreezeType ret = {
			.freezer = rank.arity > 1,
			.freezee = rank.arity == 0 && rank.defined_by.empty(),
			.visited = false,
			.args = std::vector(rank.arity,std::pair{false,false}),
		};
		if( rank.arity > 0 ) ret.args[0].first = true;
		return ret;
	}
	static FreezeType anypos( Trs::Rank const& rank ) {
		return {
			.freezer = rank.arity > 1,
			.freezee = rank.defined_by.empty(),// constructor argument is freezable
			.visited = false,
			.args = std::vector(rank.arity,std::pair{true,false}),
		};
	}
};

bool Problem::freeze( bool uncurry ) & {
	// three step freezing
	Map<std::string,FreezeType> freeze_info;
	auto init = uncurry ? FreezeType::uncurry : FreezeType::anypos;
	for( auto const& [f,finfo] : sig ) {
		freeze_info.emplace(f,init(finfo));
	}
	// step 1: enumerate freezable positions
	// TODO!! probably, a fixed point is necessary!
	Algebra<std::string,bool> prel = [&]( std::string const& f, std::vector<bool>&& args ){
		if( auto finfo = freeze_info.find(f) ) {
			auto& [freezer,freezee,visited,fargs] = *finfo;
			size_t n = fargs.size();
			assert( args.size() == n );
			visited = true;
			if( freezer ) {
				bool pot = false;
				for( uint16_t i = 0; i < n; i++ ) {
					if( args[i] ) {// freezable argument is given
						if( fargs[i].first ) {// the position is freezing
							pot = true;// potential to freeze
							continue;
						}
					}
					fargs[i].first = false;
				}
				freezer = pot;
			}
			return freezer || freezee;// freezer or freezee can be frozen further
		}
		return false;// variable cannot be frozen
	};
	Algebra<std::string,bool> prer = [&]( std::string const& f, std::vector<bool>&& args ){
		Set<std::string> ret;
		if( auto finfo = freeze_info.find(f) ) {
			auto& [freezer,freezee,visited,fargs] = *finfo;
			size_t n = fargs.size();
			assert( args.size() == n );
			if( freezer ) {
				bool pot = false;
				for( uint16_t i = 0; i < n; i++ ) {
					if( args[i] && fargs[i].first ) {// freezing happens in rhs
						fargs[i].second = true;
					}
				}
				return pot;
			}
			return freezee;
		}
		return false;
	};
	for( auto const& [i,rule] : rules ) {
		auto const& [l,r,w] = rule;
		prel(l); prer(r);
	}
	// test if any symbol can be frozen
	{
		bool freeze_any = false;
		for( auto& [f,finfo] : freeze_info ) {
			if( finfo.visited && finfo.freezer ) {
				bool freeze = false;
				for( auto [li,ri] : finfo.args ) {
					if( li && ri ) {// if lhss admit freezing the position and rhss benefit from it
						freeze = true;
					}
				}
				if( freeze ) {
					freeze_any = true;
					finfo.freezee = false;// raw freezer cannot be frozen
					ASSERTED(sig.find(f))->defined_by.clear();// forget raw symbols were defined
				} else {// do not freeze this
					finfo.freezer = false;
				}
			}
		}
		if( !freeze_any ) return false;// nothing can be frozen
	}
	// step 2: introduce frozen symbols, freeze lhs, and update defined symbols
	std::vector<Trs::Rule> freezing_rules;// freezer rules
	Algebra<std::string,::Term<std::string>> lalg =
	[&]( std::string const& f, std::vector<::Term<std::string>> const& args ){
		if( auto finfo = freeze_info.find(f) ) {
			auto& [freezer,freezee,visited,fargs] = *finfo;
			if( visited && freezer ) {
				std::string ff = "[" + f;
				std::vector<::Term<std::string>> ret_args;
				for( uint16_t i = 0; i < fargs.size(); i++ ) {
					if( fargs[i].second ) {// freezing position
						ff.append(1,' ') += args[i].fun();
						ret_args.append_range(args[i].args());
					} else {// raw position
						ff.append(" _");
						ret_args.emplace_back(args[i]);
					}
				}
				ff += ']';
				// register the frozen symbol
				auto [it,fresh] = sig.emplace(ff,Rank(ret_args.size()));
				freeze_info.emplace(ff,FreezeType{.freezee=true});// frozen symbol can be frozen further
				if( fresh ) {// constructing freezing rule f(g(x,...),y,...) -> f[g](x,...,y,...)
					uint16_t fi = 0;
					std::vector<::Term<std::string>> largs, frargs;
					for( uint16_t i = 0; i < fargs.size(); i++ ) {
						if( fargs[i].second ) {
							std::vector<::Term<std::string>> lsubargs;
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
					freezing_rules.emplace_back(Trs::Rule(app(f,std::move(largs)),app(ff,std::move(frargs))));
				}
				return app(std::move(ff),std::move(ret_args));
			}
		}
		return app(f,std::move(args));
	};
	for( auto& [i,rule] : rules ) {
		auto& [l,r,w] = rule;
		l = lalg(l);
		ASSERTED(sig.find(l.fun()))->defined_by.emplace(i);
		ASSERTED(freeze_info.find(l.fun()))->freezee=false;// do not freeze defined symbols
	}

	// step 3: freeze rhss
	Algebra<std::string,::Term<std::string>> ralg = [&]( std::string const& f, std::vector<::Term<std::string>> const& args ){
		if( auto finfo = freeze_info.find(f) ) {
			auto& [freezer,freezee,fvisited,fargs] = *finfo;
			if( freezer ) {
				std::string ff = "[" + f;
				std::vector<::Term<std::string>> ret_args;
				bool freezable = true;// check if all freezing positions are supplied constructor
				for( uint16_t i = 0; i < fargs.size(); i++ ) {
					if( fargs[i].second ) {
						auto const& g = args[i].fun();
						// freeze if the argument is freezable
						if( freeze_info.find(g) && []( auto const& ginfo ){ return ginfo.freezee; } ) {
							ff.append(1,' ') += g;
							ret_args.append_range(args[i].args());
							continue;
						}
						// freezing position but not freezable
						freezable = false;
						break;
					} else {
						ff.append(" _");
						ret_args.emplace_back(std::move(args[i]));
					}
				}
				if( freezable ) {
					ff += ']';
					sig.emplace(ff,ret_args.size());// in case the symbol appeared only in rhs
					return app(std::move(ff),std::move(ret_args));
				}
			}
		}
		return app(f,std::move(args));
	};
	for( auto& [i,rule] : rules ) {
		auto& [l,r,w] = rule;
		r = ralg(r);
	}
	// move freezer rules into main rules
	for( auto& rule : freezing_rules ) {
		ASSERTED(sig.find(rule.first.fun()))->defined_by.emplace(next_rule);
		insert_rule(std::move(rule));
	}
	return true;
}
