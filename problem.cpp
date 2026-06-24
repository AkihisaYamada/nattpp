#include<fstream>
#include"problem.hpp"
#include"reach.hpp"

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
void Problem::insert_rule( Trs::Rules& rules, Trs::Rule const& rule ) & {
	rules.emplace(next_rule,rule);
	next_rule++;
}

bool Problem::reads_sym_decl( Reader& eis ) & {
	if( eis.reads_sym("fun") ) {
		string fun = eis.read_sym();
		unsigned char arity = 255;
		while( auto key = eis.reads_key() ) {
			if( *key == ":arity" ) {
				if( arity != 255 ) {
					throw Error{"#duplicate-arity",fun};
				}
				arity = eis.read_nat();
			} else {
				throw Error{"#unknown-key",*key};
			}
		}
		if( auto num = eis.reads_nat() ) {
			if( arity != 255 ) {
				throw Error("#duplicate-arity",fun,to_string(*num));
			}
			arity = *num;
		}
		if( auto [prev,suc] = main.sig.emplace(fun,Trs::Rank{arity,false}); !suc ) {
			throw Error{"#duplicate-fun",fun,to_string(prev.arity),to_string(arity)};
		}
		return true;
	}
	return false;
}

bool Problem::reads_rule_decl( Reader& eis, Trs::Reader& tis ) & {
	if( eis.reads_sym("rule") ) {
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
		if( !index || *index == 1 ) {// main rule
			if( auto rank = main.sig.find(l.fun()) ) {
				rank->defined_by.emplace(next_rule);
			}
			insert_rule(main.rules,std::move(rule));
		} else {
			insert_rule(components[*index-2].rules,std::move(rule));
		}
		return true;
	}
	return false;
}
Problem::Problem( istream& is ) : next_rule(0), uses_graph(uses_map) {
	auto eis = Reader(is);
	mode = NONE;
	eis.open();
	if( eis.reads_sym("format") ) {
		if( eis.reads_sym("TRS") ) {
			format = TRS;
			Opt<int> number;
			while( auto key = eis.reads_key() ) {
				if( *key == ":number" ) {
					if( number ) throw eis.error("#duplicate-number");
					number = {eis.read_nat([](auto n){ return n < 10; })};
				} else if( *key == ":problem" ) {
					if( mode != NONE ) throw eis.error("#duplicate-problem");
					if( eis.reads_sym("sat") ) {
						mode = SAT;
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
				if( !reads_sym_decl(eis) && !reads_rule_decl(eis,tis) ) {
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
	Problem& p, Set<size_t>& org_uses, Map<size_t,Set<size_t>>& dp_uses
) {
	if( auto rrank = sig.find(r.fun()) ) {// f(...) -> g(...)
		if( rrank->defined_by.empty() ) {// just look arguments
			for( auto const& a : r.args() ) {
				collect_dps(sig,dps,l,lrank,a,p,org_uses,dp_uses);
			}
		} else {// g is defined
			Set<size_t> this_uses;// collect rules which this dp uses
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
			for( size_t i : rrank->defined_by ) {// the origin also uses the rules that define g
				if( auto const& rule = p.main.rules.find(i) ) {
					auto const& [l2,r2,w] = *rule;
					if( may_reach(p.main,r,l2,8) ) {
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
	Map<size_t,Set<size_t>> dp_uses;
	for( auto const& [org,rule] : main.rules ) {
		auto const& l = rule.first;
		auto lrank = main.sig.find(l.fun());
		if( !lrank ) {
			cerr << "(var-lhs " << org << ')' << endl;
			throw Answer::NO;
		}
		Set<size_t> uses;
		collect_dps(main.sig,dps,l,*lrank,rule.second,*this,uses,dp_uses);
		uses_map.emplace(org,std::move(uses));// register rules that the original uses
	}
	// complete dp_usables
	auto const& utr = uses_graph.trancl();// usability graph is completed
	for( auto const& [dp,uses] : dp_uses ) {
		auto [usables,fl] = dp_usables.emplace(dp,Set<size_t>());
		assert(fl);
		for( auto const& use : uses ) {
			utr.iter_nexts(use,[&]( auto const& x ){
				usables.emplace(x);
			});
			usables.emplace(use);
		}
	}
}
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
		dp_usables.emplace( next_rule, ASSERTED(dp_usables.extract(uind)).mapped() );
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
