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

Problem::Problem( istream& is ) : next_rule(0) {
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
		auto& [sig,rules] = systems.emplace_back();
		Trs::SigFun sig_fun = [&](string const& sym ){ return sig.find(sym); };
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
				if( auto [prev,suc] = sig.emplace(fun,Trs::Rank{arity,false}); !suc ) {
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
						int i = eis.read_nat([&](auto n){ return 0 < n && n <= systems.size(); });
						index = {i};
					} else {
						throw Error{"#unknown-key",*key};
					}
				}
				if( auto rank = sig.find(l.fun()) ) {
					rank->defined = true;
				}
				insert_rule( systems.back().rules, Trs::Rule(l,r,weight.value_or(1)) );
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
	Problem& p
) {
	if( auto rrank = sig.find(r.fun()) )
		if( rrank->defined ) {
			lrank.depends.emplace(p.next_rule);
			p.insert_rule(rules,Trs::Rule(l,r));
		}
	for( auto const& a : r.args() ) {
		collect_dps(sig,rules,l,lrank,a,p);
	}
}

void Problem::make_dps() & {
	assert( systems.size() == 1 );
	assert( mode == SN );
	auto& [dpsig,dps] = systems.emplace_back();
	mode = DP;
	auto& [sig,rules] = systems.front();
	Pos pos;
	for( auto const& [org,rule] : rules ) {
		auto const& l = rule.first;
		auto lrank = sig.find(l.fun());
		if( !lrank ) {
			cerr << "(var-lhs " << org << ')' << endl;
			throw Answer::NO;
		}
		collect_dps(sig,dps,l,*lrank,rule.second,*this);
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
void Problem::mark_dps( SysIt const& it ) & {
	auto mdps = Trs::Rules();
	auto& [msig,udps] = *it;
	auto const& sig = systems.front().sig;
	assert(msig.empty());
	for( auto uit = udps.begin(); uit != udps.end(); uit = udps.erase(uit) ) {// iterate while removing
		insert_rule(mdps,mark_dp(sig,msig,uit->second));
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
	if( systems.empty() ) return os;
	auto it = systems.begin();
	for( auto const& [f,rank] : it->sig ) {
		os << "\n  (fun " << f << ' ' << rank << ')' << flush;
	}
	for( auto const& [n,rule] : it->rules ) {
		os << "\n  (rule-n " << n << ' ' << rule.print_content() << ')' << flush;
	}
	int sysno = 1;
	while( it++, it != systems.end() ) {
		auto const& [sig,rules] = *it;
		sysno++;
		os << "\n  (set-n " << sysno;
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
