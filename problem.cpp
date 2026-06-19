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

Problem::Problem( istream& is ) : last_ind(0) {
	auto eis = Reader(is);
	eis.open();
	eis.read_sym("format");
	if( eis.reads_sym("TRS") ) {
		format = TRS;
		Opt<int> number;
		while( auto key = eis.reads_key() ) {
			if( *key == ":number" ) {
				if( number ) throw eis.error("#duplicate-number");
				number = {eis.read_nat([](auto n){ return n < 10; })};
			} else {
				throw Error("#unknown-key",*key);
			}
		}
		eis.close();// of format
		systems = vector<Trs::Rules>( number ? *number : 1 );
		auto tis = Trs::Reader(eis,sig);
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
						extra_var.emplace(last_ind,var);
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
				systems[ index ? *index-1 : 0 ].emplace( last_ind, Trs::Rule(l, r, weight ? *weight : 1) );
				last_ind++;
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

ostream& Problem::print( ostream& os ) const& {
	os << "(problem";
	switch( mode ) {
		case SN: os << " termination"; break;
		case DP: os << " DP-termination"; break;
		default: assert(false);
	}
	os << flush;
	if( systems.empty() ) return os;
	for( auto const& [f,rank] : sig ) {
		os << "\n  (fun " << f << ' ' << rank << ')' << flush;
	}
	for( auto const& [n,rule] : systems[0] ) {
		os << "\n  (rule-n " << n << ' ' << rule.print_content() << ')' << flush;
	}
	int sysno = 1;
	while( sysno < systems.size() ) {
		auto const& rules = systems[sysno];
		sysno++;
		for( auto const& [n,rule] : rules ) {
			os << "\n  (rule-n " << rule.print_content() << " :index " << sysno << ')' << flush;
		}
	}
	return os << ')';
}

bool Problem::test() {
	{
		auto ifs = ifstream("samples/add.ari");
		auto prob = Problem(ifs);
		for( int i = 0; i < prob.systems.size(); i++ ) {
			cout << "TRS " << i+1 << ":" << endl << prob.systems[i];
		}
	}
	return true;
}
