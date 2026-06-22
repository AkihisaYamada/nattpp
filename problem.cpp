#include<fstream>
#include"problem.hpp"

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

Problem::Problem( istream& is ) : next_rule(0), usable_graph(usable_map) {
	auto eis = Reader(is);
	eis.open();
	eis.read_sym("format");
	if( eis.reads_sym("TRS") ) {
		format = TRS;
		Opt<int> number;
		while( auto key = eis.reads_key() ) {
			if( *key == ":number" ) {
				throw eis.error("#unsupported",*key);
				if( number ) throw eis.error("#duplicate-number");
				number = {eis.read_nat([](auto n){ return n < 10; })};
			} else {
				throw Error("#unknown-key",*key);
			}
		}
		eis.close();// of format
		Trs::SigFun sig_fun = [&](string const& sym ){ return main.sig.find(sym); };
		auto tis = Trs::Reader(eis,sig_fun);
		while( eis.opens() ) {
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
			} else if( eis.reads_sym("rule") ) {
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
						int i = eis.read_nat([&](auto n){ return 0 < n && n <= subtrss.size()+1; });
						index = {i};
					} else {
						throw Error{"#unknown-key",*key};
					}
				}
				if( auto rank = main.sig.find(l.fun()) ) {
					rank->defined_by.emplace(next_rule);
				}
				insert_rule( main.rules, Trs::Rule(l,r,weight.value_or(1)) );
			} else {
				throw Error{"#unknown-command",eis.read_exp()};
			};
			eis.close();
		}
		mode = SN;
	} else {
		throw Error("#unsupported-format",eis.read_exp());
	}
}

static void collect_dps(
	Trs::Sig& sig, Trs::Rules& rules,
	Trs::Term const& l, Trs::Rank& lrank, Trs::Term const& r,
	Problem& p, Set<size_t>& org_uses
) {
	if( auto rrank = sig.find(r.fun()) ) {// f(...) -> g(...)
		if( !rrank->defined_by.empty() ) {// g is defined
			Set<size_t> this_uses;// collect rules which this dp uses
			for( auto const& a : r.args() ) {
				collect_dps(sig,rules,l,lrank,a,p,this_uses);
			}
			lrank.depends.emplace(p.next_rule);// assign this dp to f
			for( auto const& used : this_uses ) {
				org_uses.emplace(used);// origin uses those rules which this dp uses
			}
			// register those this dp will use
			p.usable_map.emplace(p.next_rule,std::move(this_uses));
			p.insert_rule(rules,Trs::Rule(l,r));
			for( size_t i : rrank->defined_by ) {// the origin also uses the rules that define g
				org_uses.emplace(i);
			}
		}
	}
}

void Problem::make_dps() & {
	assert( mode == SN );
	auto& [dpsig,dps] = subtrss.emplace_back();
	mode = DP;
	Pos pos;
	for( auto const& [org,rule] : main.rules ) {
		auto const& l = rule.first;
		auto lrank = main.sig.find(l.fun());
		if( !lrank ) {
			cerr << "(var-lhs " << org << ')' << endl;
			throw Answer::NO;
		}
		Set<size_t> uses;
		collect_dps(main.sig,dps,l,*lrank,rule.second,*this,uses);
		usable_map.emplace(org,std::move(uses));// register rules that the original uses
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
void Problem::mark_dps( SubIt const& it ) & {
	auto mdps = Trs::Rules();
	auto& [msig,udps] = *it;
	assert(msig.empty());
	for( auto uit = udps.begin(); uit != udps.end(); uit = udps.erase(uit) ) {// iterate while removing
		auto [uind,udp] = *uit;
		// marked dp will use what has been used by the unmarked one
		usable_map.emplace( next_rule, ASSERTED(usable_map.extract(uind)).mapped() );
		insert_rule(mdps,mark_dp(main.sig,msig,udp));
	}
	swap(mdps,udps);
}

ostream& Problem::print( ostream& os ) const& {
	os << "(problem";
	switch( mode ) {
		case SN: os << " termination"; break;
		case DP: os << " dp"; break;
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
	for( auto it = subtrss.begin(); it != subtrss.end(); it++ ) {
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
