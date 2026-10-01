#include<fstream>
#include"problem.hpp"
#include"reach.hpp"
#include"graph.hpp"

using namespace std;

Answer const Answer::YES = Answer("YES"), Answer::NO = Answer("NO"), Answer::MAYBE("MAYBE");

static void _switch(
	string const& key,
	map<string,function<void(void)>> map,
	function<void(string const&)> other
) {
	auto it = map.find(key);
	if( it == map.end() ) {
		other(key);
	} else {
		it->second();
	}
}
void Problem::insert_rule( Trs::Rules& rules, Trs::Rule const& rule, uint32_t rule_ind ) & {
	auto [ref,fl] = rules.emplace(rule_ind,rule);
	if( !fl ) throw Error("#duplicate-rule-no",to_string(rule_ind));
	next_rule = std::max(next_rule,rule_ind) + 1;
}
void Problem::insert_rule( Trs::Rules& rules, Trs::Rule const& rule ) & {
	rules.emplace(next_rule,rule);
	next_rule++;
}

bool Problem::reads_sym_decl( Reader& eis ) & {
	if( eis.reads_sym("fun") ) {
		string fun = eis.read_sym();
		unsigned char arity = 255;
		Opt<uint32_t> index = {};
		if( auto num = eis.reads_nat() ) {
			if( arity != 255 ) {
				throw Error("#duplicate-arity",fun,to_string(*num));
			}
			arity = *num;
		}
		while( auto key = eis.reads_key() ) {
			if( *key == ":arity" ) {
				if( arity != 255 ) {
					throw eis.error("#duplicate-arity",fun);
				}
				arity = eis.read_nat([](auto n){ return n < 255; });
			} else if( *key == ":index" ) {
				if( index ) throw eis.error("#duplicate-index",fun);
				index = {eis.read_nat()};
			} else {
				throw eis.error("#unknown-key",*key);
			}
		}
		if( auto [prev,suc] = main.sig.emplace(fun,Trs::Rank{arity,false}); !suc ) {
			throw Error{"#duplicate-fun",fun,to_string(prev.arity),to_string(arity)};
		}
		return true;
	}
	return false;
}

void Problem::read_rule_decl( Reader& eis, Trs::Reader& tis ) & {
		std::set<std::string> vars;
		auto l = tis.read([&]( auto const& var ){
			vars.emplace(var);
		});
		auto r = tis.read([&]( auto const& var ){
			if( !vars.contains(var) ) {
				extra_var.emplace(next_rule,var);
			}
		});
		Opt<int> weight;
		Opt<uint32_t> rule_indo;
		Opt<int> set_index;
		while( auto key = eis.reads_key() ) {
			if( *key == ":cost" ) {
				if( weight ) throw Error{"#duplicate-cost"};
				int i = eis.read_int();
				weight = {i};
			} else if( *key == ":index" ) {
				if( set_index ) throw Error{"#duplicate-index"};
				int i = eis.read_nat([&](auto n){ return 0 < n && n <= components.size()+1; });
				set_index = {i};
			} else if( *key == ":number" ) {
				if( rule_indo ) throw Error{"#duplicate-rule-number"};
				rule_indo = {eis.read_nat()};
			} else {
				throw Error{"#unknown-key",*key};
			}
		}
		auto const& rule = Trs::Rule(l,r,weight.value_or(1));
		uint32_t rule_ind = rule_indo ? *rule_indo : next_rule;
		auto& rules = [&]()->Trs::Rules&{
			if( !set_index || *set_index == 1 ) {// main rule
				if( auto rank = main.sig.find(l.fun()) ) {
					rank->defined_by.emplace(rule_ind);
				}
				return main.rules;
			} else {
				return components[*set_index-2].nodes;
			};
		}();
		if( rule_indo ) {
			insert_rule(rules,std::move(rule),rule_ind);
		} else {
			insert_rule(rules,std::move(rule));
		}
}
Problem::Problem( istream& is ) : next_rule(0) {
	auto eis = Reader(is);
	mode = NONE;
	eis.open();
	if( eis.reads_sym("format") ) {
		if( eis.reads_sym("TRS") ) {
			format = TRS;
			Opt<unsigned int> number;
			while( auto key = eis.reads_key() ) {
				if( *key == ":number" ) {
					if( number ) throw eis.error("#duplicate-number");
					number = {eis.read_nat([](auto n){ return n < 10; })};
				} else if( *key == ":problem" ) {
					if( mode != NONE ) throw eis.error("#duplicate-problem");
					if( eis.reads_sym("sat") ) {
						mode = SAT;
					} else if( eis.reads_sym("dp") ) {
						mode = DP;
					} else {
						throw eis.error("#unknown-problem");
					}
				} else {
					throw Error("#unknown-key",*key);
				}
			}
			eis.close();// of format
			Trs::SigFun sig_fun = [&](string const& sym ){ return main.sig.find(sym); };
			if( number && *number > 1 ) {
				components = std::deque<Component>(*number-1);
			}
			auto tis = Trs::Reader(eis,sig_fun);
			while( eis.opens() ) {
				if( reads_sym_decl(eis) ) {
				} else if( eis.reads_sym("rule") ) {
					read_rule_decl(eis,tis);
				} else {
					throw Error{"#unknown-command",eis.read_exp()};
				};
				eis.close();
			}
		} else {
			throw Error("#unsupported-format",eis.read_exp());
		}
	}
}

string mark_sym( string const& sym ) {
	return string("#")+sym;
}

static void collect_dps(
	Trs::Sig& sig, Trs::Rules& dps,
	Trs::Term const& L, Trs::Rank& Finfo, Trs::Term const& r,
	Problem& p, Set<uint32_t>& org_uses,
	Map<uint32_t,Set<uint32_t>>& dp_uses
) {
	auto const& [g,rs] = *r;
	auto ginfo = sig.find(g);
	if( !ginfo || ginfo->defined_by.empty() ) {// rhs is a variable or constructor
		for( auto const& a : rs ) {// just look into arguments
			collect_dps(sig,dps,L,Finfo,a,p,org_uses,dp_uses);
		}
	} else {// rhs is defined
		auto G = mark_sym(g);
		// record here rules this dp will use
		auto& this_uses = dp_uses.emplace(p.next_rule,Set<uint32_t>{}).first;
		Finfo.defined_by.emplace(p.next_rule);// assign this dp to f
		p.insert_rule(dps,Trs::Rule(L,app(G,rs)));
		for( auto const& a : rs ) {// look into arguments
			collect_dps(sig,dps,L,Finfo,a,p,this_uses,dp_uses);
		}
		for( auto const& used : this_uses ) {
			org_uses.emplace(used);// origin uses those rules which this dp uses
		}
		for( uint32_t i : ginfo->defined_by ) {// the origin also uses the rules that define g
			if( auto const& rule = p.main.rules.find(i) ) {
				auto const& [l2,r2,w] = *rule;
				if( may_reach(p.main,r,l2,8,false) ) {
					org_uses.emplace(i);
				}
			}
		}
	}
}

void Problem::make_dps() & {
	assert( mode == SN );
	auto& [dps,dg] = components.emplace_back();
	mode = DP;
	// make marked signature
	Trs::Sig msig;
	for( auto const& [f,finfo] : main.sig ) {
		if( !finfo.defined_by.empty() ) {
			msig.emplace(mark_sym(f),Trs::Rank(finfo.arity));
		}
	}
	main.sig.merge(msig);// note: `merge` takes elements from msig.
	// compute DPs
	for( auto const& [org,rule] : main.rules ) {
		auto const& [l,r,w] = rule;
		auto const& [f,ls] = *l;
		auto finfo = main.sig.find(f);
		if( !finfo ) {
			cerr << "(var-lhs " << org << ')' << endl;
			throw Answer::NO;
		}
		auto F = mark_sym(f);
		Set<uint32_t> uses;// collect here rules that the original uses
		collect_dps(main.sig,dps,app(F,ls),*ASSERTED(main.sig.find(F)),r,*this,uses,uses_map);
		uses_map.emplace(org,std::move(uses));
	}
	usable_graph = ConstGraph(uses_map).trancl();
	// creating dependency graph
	for( auto const& [i,dp] : dps ) {
		auto const& [L1,R1,W1] = dp;
		auto [nexts,fl] = dg.emplace(i,Set<uint32_t>{});
		for( auto const& j : ASSERTED(main.sig.find(R1.fun()))->defined_by ) {
			auto const& [L2,R2,W2] = *ASSERTED(dps.find(j));
			if( may_reach(main,R1,L2,8,false) ) {
				nexts.emplace(j);
			}
		}
	};
}

static void term_use(
	Trs const& trs,
	Trs::Term const& s,
	Set<uint32_t>& uses
) {
	auto const& [f,ss] = *s;
	for( auto const& a : ss ) {// use subterms
		term_use(trs,a,uses);
	}
	if( auto rrank = trs.sig.find(f) ) {
		for( uint32_t i : rrank->defined_by ) {// uses the rules that define the root
			if( auto const& rule = trs.rules.find(i) ) {
				auto const& [l,r,w] = *rule;
				if( may_reach(trs,s,l,8,false) ) {
					uses.emplace(i);
				}
			}
		}
	}
}
void Problem::init_uses() & {
	assert( mode == DP );
	for( auto const& [i,rule] : main.rules ) {
		auto const& [l,r,w] = rule;
		auto uses = Set<uint32_t>{};
		term_use(main,r,uses);
		uses_map.emplace(i,std::move(uses));
	}
	for( auto const& comp : components ) {
		for( auto const& [i,dp] : comp.nodes ) {
			auto const& [l,r,w] = dp;
			auto uses = Set<uint32_t>{};
			term_use(main,r,uses);
			uses_map.emplace(i,std::move(uses));
		}
	}
	usable_graph = ConstGraph(uses_map).trancl();
};

void Problem::decomp_sccs() & {
	assert( mode == DP );
	if( components.empty() ) return;// nothing to decompose
	auto [dps,dgmap] = std::move(components.front());
	components.pop_front();
	auto dg = Graph(dgmap);
	auto sccs = dg.sccs();
	for( uint32_t i = 0; i < sccs.size(); i++ ) {
		if( auto const& nodes = sccs[i].ref<Set<uint32_t>>() ) {// nontrivial SCCs
			auto& back = components.emplace_back();
			for( auto const& dpind : *nodes ) {
				if( auto const& dpop = dps.find(dpind) ) {
					back.nodes.emplace(dpind,*dpop);
					auto& nexts = back.graph.emplace(dpind,Set<uint32_t>{}).first;
					dg.iter_nexts(dpind,[&]( uint32_t next ){// copy edges inside SCC
						if( dg.scc_ind(next) == i ) {
							nexts.emplace(next);
						}
					});
				}
			}
		}
	}
}

ostream& Problem::print( ostream& os ) const& {
	os << "(problem";
	switch( mode ) {
		case SN: case NONE: os << " termination"; break;
		case DP: os << " dp"; break;
		case SAT: os << " sat"; break;
		default: assert(false);
	}
	os << flush;
	for( auto const& [f,rank] : main.sig ) {
		os << "\n  (fun " << f << ' ' << rank << ')' << flush;
	}
	for( auto const& [n,rule] : main.rules ) {
		os << "\n  (rule " << rule.print_content() << " :number " << n << ')' << flush;
	}
	int subno = 1;
	for( auto it = components.begin(); it != components.end(); it++ ) {
		auto const& [rules,graph] = *it;
		os << "\n  (component";
		for( auto const& [n,rule] : rules ) {
			os << "\n    (rule " << rule.print_content() << " :number " << n << ')' << flush;
		}
		os << "\n    (edges" << ConstGraph(graph).print_nodes("\n      ") << ")"
		   << "\n    :number " << subno << ')' << flush;
		subno++;
	}
	return os << ')';
}

bool Problem::test() {
	{
		auto ifs = ifstream("samples/add.ari");
		auto prob = Problem(ifs);
		cout << prob << endl;
	}
	return true;
}
