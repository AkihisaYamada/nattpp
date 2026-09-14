#include<fstream>
#include"problem.hpp"
#include"reach.hpp"
#include"graph.hpp"

using namespace std;

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
		auto& sig = [&]()->Trs::Sig&{
			if( index && *index > 1 ){
				return components[*index-2].sig;
			}
			return main.sig;
		}();
		if( auto [prev,suc] = sig.emplace(fun,Trs::Rank{arity,false}); !suc ) {
			throw Error{"#duplicate-fun",fun,to_string(prev.arity),to_string(arity)};
		}
		return true;
	}
	return false;
}

void Problem::read_rule_decl( Reader& eis, Trs::Reader& tis, Opt<uint32_t> rule_indo ) & {
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
		Opt<int> index;
		while( auto key = eis.reads_key() ) {
			if( *key == ":cost" ) {
				if( weight ) throw Error{"#duplicate-cost"};
				int i = eis.read_int();
				weight = {i};
			} else if( *key == ":index" ) {
				if( index ) throw Error{"#duplicate-index"};
				int i = eis.read_nat([&](auto n){ return 0 < n && n <= components.size()+1; });
				index = {i};
			} else {
				throw Error{"#unknown-key",*key};
			}
		}
		auto const& rule = Trs::Rule(l,r,weight.value_or(1));
		uint32_t rule_ind = rule_indo ? *rule_indo : next_rule;
		auto& rules = [&]()->Trs::Rules&{
			if( !index || *index == 1 ) {// main rule
				if( auto rank = main.sig.find(l.fun()) ) {
					rank->defined_by.emplace(rule_ind);
				}
				return main.rules;
			} else {
				return components[*index-2].rules;
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
				components = std::deque<Trs>(*number-1);
			}
			auto tis = Trs::Reader(eis,sig_fun);
			while( eis.opens() ) {
				if( reads_sym_decl(eis) ) {
				} else if( eis.reads_sym("rule") ) {
					read_rule_decl(eis,tis,{});
				} else if( eis.reads_sym("rule-n") ) {
					uint32_t rule_ind = eis.read_nat();
					read_rule_decl(eis,tis,rule_ind);
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

static void collect_dps(
	Trs::Sig& sig, Trs::Rules& dps,
	Trs::Term const& l, Trs::Rank& lrank, Trs::Term const& r,
	Problem& p, Set<uint32_t>& org_uses, Map<uint32_t,Set<uint32_t>>& dp_uses
) {
	for( auto const& a : r.args() ) {// first look arguments
		collect_dps(sig,dps,l,lrank,a,p,org_uses,dp_uses);
	}
	if( auto rrank = sig.find(r.fun()) ) {// f(...) -> g(...)
		if( !rrank->defined_by.empty() ) {
			Set<uint32_t> this_uses;// collect rules which this dp uses
			for( auto const& a : r.args() ) {
				collect_dps(sig,dps,l,lrank,a,p,this_uses,dp_uses);
			}
			lrank.depends.emplace(p.next_rule);// assign this dp to f
			for( auto const& used : this_uses ) {
				org_uses.emplace(used);// origin uses those rules which this dp uses
			}
			// register those this dp will use
			dp_uses.emplace(p.next_rule,std::move(this_uses));
			p.insert_rule(dps,Trs::Rule(l,r));
			for( uint32_t i : rrank->defined_by ) {// the origin also uses the rules that define g
				if( auto const& rule = p.main.rules.find(i) ) {
					auto const& [l2,r2,w] = *rule;
					if( may_reach(p.main,r,l2,8,false) ) {
						org_uses.emplace(i);
					}
				}
			}
		}
	}
}

void Problem::make_dps() & {
	assert( mode == SN );
	auto& [dpsig,dps] = components.emplace_back();
	mode = DP;
	Pos pos;
	for( auto const& [org,rule] : main.rules ) {
		auto const& l = rule.first;
		auto lrank = main.sig.find(l.fun());
		if( !lrank ) {
			cerr << "(var-lhs " << org << ')' << endl;
			throw Answer::NO;
		}
		Set<uint32_t> uses;// collect here rules that the original uses
		collect_dps(main.sig,dps,l,*lrank,rule.second,*this,uses,uses_map);
		uses_map.emplace(org,std::move(uses));
	}
	usable_graph = ConstGraph(uses_map).trancl();
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
		for( auto const& [i,dp] : comp.rules ) {
			auto const& [l,r,w] = dp;
			auto uses = Set<uint32_t>{};
			term_use(main,r,uses);
			uses_map.emplace(i,std::move(uses));
		}
	}
	usable_graph = ConstGraph(uses_map).trancl();
};

string mark_sym( string const& sym ) {
	return string("#")+sym;
}
Trs::Rule mark_dp( Trs::Sig const& sig, Trs::Sig& extra_sig, Trs::Rule const& dp ) {
	auto const& l = dp.first, &r = dp.second;
	auto lfm = mark_sym(l.fun()), rfm = mark_sym(r.fun());
	if( !extra_sig.find(lfm) ) {
		extra_sig.emplace(lfm,*ASSERTED(sig.find(l.fun())));
	}
	if( !extra_sig.find(rfm) ) {
		extra_sig.emplace(rfm,*ASSERTED(sig.find(r.fun())));
	}
	return Trs::Rule(app(lfm,l.args()),app(rfm,r.args()));
}
void Problem::mark_dps() & {
	auto mdps = Trs::Rules();
	auto& [msig,udps] = components.front();
	assert(msig.empty());
	for( auto uit = udps.begin(); uit != udps.end(); uit = udps.erase(uit) ) {// iterate while removing
		auto [uind,udp] = *uit;
		// marked dp will use what has been used by the unmarked one
		usable_graph.emplace( next_rule, ASSERTED(usable_graph.extract(uind)).mapped() );
		insert_rule(mdps,mark_dp(main.sig,msig,udp));
	}
	swap(mdps,udps);
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
		os << "\n  (rule-n " << n << ' ' << rule.print_content() << ')' << flush;
	}
	int subno = 1;
	for( auto it = components.begin(); it != components.end(); it++ ) {
		auto const& [sig,rules] = *it;
		subno++;
		os << "\n  (set-n " << subno;
		for( auto const& [f,rank] : sig ) {
			os << "\n    (fun " << f << ' ' << rank << ')' << flush;
		}
		for( auto const& [n,rule] : rules ) {
			os << "\n    (rule-n " << n << ' ' << rule.print_content() << ')' << flush;
		}
		os << ')' << flush;
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
