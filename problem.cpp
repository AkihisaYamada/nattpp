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
void Problem::insert_rule( Trs::Rule const& rule, ruleId rule_ind ) & {
	auto [ref,fl] = rules.emplace(rule_ind,rule);
	if( !fl ) throw Error("#duplicate-rule-no",to_string(rule_ind));
	next_rule = std::max(next_rule,rule_ind) + 1;
}
void Problem::insert_rule( Trs::Rule const& rule ) & {
	rules.emplace(next_rule,rule);
	next_rule++;
}

bool Problem::reads_sym_decl( ::Reader& eis ) & {
	if( eis.reads_sym("fun") ) {
		string fun = eis.read_sym();
		unsigned char arity = 255;
		Opt<ruleId> index = {};
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
				index = {ruleId(eis.read_nat())};
			} else {
				throw eis.error("#unknown-key",*key);
			}
		}
		if( auto [prev,suc] = sig.emplace(fun,Trs::Rank{arity,false}); !suc ) {
			throw Error{"#duplicate-fun",fun,to_string(prev.arity),to_string(arity)};
		}
		return true;
	}
	return false;
}

void Problem::read_rule_decl( ::Reader& eis, Trs::Reader& tis ) & {
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
		Opt<ruleId> rule_indo;
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
				rule_indo = {ruleId(eis.read_nat())};
			} else {
				throw Error{"#unknown-key",*key};
			}
		}
		auto const& rule = Trs::Rule(l,r,weight.value_or(1));
		ruleId rule_ind = rule_indo ? *rule_indo : next_rule;
		if( auto rank = sig.find(l.fun()) ) {
			rank->defined_by.emplace(rule_ind);
		}
		if( set_index && *set_index != 1 ) {// main rule
			throw Error("#unsupported-index");
		}
		if( rule_indo ) {
			insert_rule(std::move(rule),rule_ind);
		} else {
			insert_rule(std::move(rule));
		}
}
Problem::Problem( istream& is ) : next_rule(ruleId(0)) {
	auto eis = ::Reader(is);
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
			Trs::SigFun sig_fun = [&](string const& sym ){ return sig.find(sym); };
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
	Trs::Sig& sig, Set<ruleId>& dps,
	Trs::Term const& L, Trs::Rank& Finfo, Trs::Term const& r,
	Problem& p, Set<ruleId>& org_uses,
	Map<ruleId,Set<ruleId>>& dp_uses
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
		auto& this_uses = dp_uses.emplace(p.next_rule,Set<ruleId>{}).first;
		Finfo.defined_by.emplace(p.next_rule);// assign this dp to f
		dps.emplace(p.next_rule);
		p.insert_rule(Trs::Rule(L,app(G,rs)));
		for( auto const& a : rs ) {// look into arguments
			collect_dps(sig,dps,L,Finfo,a,p,this_uses,dp_uses);
		}
		for( auto const& used : this_uses ) {
			org_uses.emplace(used);// origin uses those rules which this dp uses
		}
		for( ruleId i : ginfo->defined_by ) {// the origin also uses the rules that define g
			if( auto const& rule = p.rules.find(i) ) {
				auto const& [l2,r2,w] = *rule;
				if( may_reach(p,r,l2,8,false) ) {
					org_uses.emplace(i);
				}
			}
		}
	}
}

void Problem::make_dps() & {
	assert( mode == SN );
	mode = DP;
	// make marked signature
	Trs::Sig msig;// can't insert while iterating over it
	for( auto const& [f,finfo] : sig ) {
		if( !finfo.defined_by.empty() ) {
			msig.emplace(mark_sym(f),Trs::Rank(finfo.arity));
		}
	}
	sig.merge(msig);// note: `merge` takes elements from msig.
	for( auto const& [i,rule] : rules ) {// remember original rule ids
		rule_ids.emplace(i);
	}
	// compute DPs
	Set<ruleId> dps;
	for( auto const& org : rule_ids ) {
		auto const& [l,r,w] = *ASSERTED(rules.find(org));
		auto const& [f,ls] = *l;
		auto finfo = sig.find(f);
		if( !finfo ) {
			cerr << "(var-lhs " << org << ')' << endl;
			throw Answer::NO;
		}
		auto F = mark_sym(f);
		Set<ruleId> uses;// collect here rules that the original uses
		collect_dps(sig,dps,app(F,ls),*ASSERTED(sig.find(F)),r,*this,uses,uses_map);
		uses_map.emplace(org,std::move(uses));
	}
	usable_graph = ConstGraph(uses_map).trancl();
	// creating dependency graph
	auto& component = components.emplace_back();
	for( auto i : dps ) {
		auto const& [L1,R1,W1] = *ASSERTED(rules.find(i));
		auto [nexts,fl] = component.graph.emplace(i,Set<ruleId>{});
		for( auto const& j : ASSERTED(sig.find(R1.fun()))->defined_by ) {
			auto const& [L2,R2,W2] = *ASSERTED(rules.find(j));
			if( may_reach(*this,R1,L2,8,false) ) {
				nexts.emplace(j);
			}
		}
	};
}

static void term_use(
	Trs const& trs,
	Trs::Term const& s,
	Set<ruleId>& uses
) {
	auto const& [f,ss] = *s;
	for( auto const& a : ss ) {// use subterms
		term_use(trs,a,uses);
	}
	if( auto rrank = trs.sig.find(f) ) {
		for( ruleId i : rrank->defined_by ) {// uses the rules that define the root
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
	for( auto const& [i,rule] : rules ) {
		auto const& [l,r,w] = rule;
		auto uses = Set<ruleId>{};
		term_use(*this,r,uses);
		uses_map.emplace(i,std::move(uses));
	}
	for( auto const& comp : components ) {
		for( auto const& [i,js] : comp.graph ) {
			auto const& [l,r,w] = *ASSERTED(rules.find(i));
			auto uses = Set<ruleId>{};
			term_use(*this,r,uses);
			uses_map.emplace(i,std::move(uses));
		}
	}
	usable_graph = ConstGraph(uses_map).trancl();
};

void Problem::decomp_sccs() & {
	assert( mode == DP );
	if( components.empty() ) return;// nothing to decompose
	auto component = std::move(components.front());
	components.pop_front();
	auto dg = Graph(component.graph);
	auto sccs = dg.sccs();
	for( uint32_t i = 0; i < sccs.size(); i++ ) {
		if( auto const& nodes = sccs[i].ref<Set<ruleId>>() ) {// nontrivial SCCs
			auto& back = components.emplace_back();
			for( auto const& dpind : *nodes ) {
				auto& nexts = back.graph.emplace(dpind,Set<ruleId>{}).first;
				dg.iter_nexts(dpind,[&]( ruleId next ){// copy edges inside SCC
					if( dg.scc_ind(next) == i ) {
						nexts.emplace(next);
					}
				});
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
	Trs::print_sig(os,sig,"\n  ");
	Trs::print_rules(os,rules,"\n  ");
	int subno = 1;
	for( auto const& c : components ) {
		os << "\n  (component";
		for( auto const& [n,rule] : rules ) {
			os << "\n    (rule " << rule.print_content() << " :number " << n << ')' << flush;
		}
		os << "\n    (edges" << ConstGraph(c.graph).print_nodes("\n      ") << ")"
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
