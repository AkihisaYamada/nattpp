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

Problem::Problem( istream& is ) {
	auto eis = Reader(is);
	eis.open();
	eis.read_sym("format");
	if( eis.reads_sym("TRS") ) {
		format = TRS;
		Opt<int> number;
		while( auto key = eis.reads_key() ) {
			if( *key == ":number" ) {
				unsigned int n = eis.read_nat();
				if( n > 9 ) throw Error("#too-big-number",to_string(n));
				number = {n};
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
				if( auto prev = sig.insert(fun,Trs::Rank{arity}) ) {
					throw Error{"#duplicate-fun",fun,to_string(prev->arity),to_string(arity)};
				}
			} else if( eis.reads_sym("rule") ) {
				auto l = tis.read(), r = tis.read();
				Opt<int> weight;
				Opt<int> index;
				while( auto key = eis.reads_key() ) {
					if( *key == ":weight" ) {
						if( weight ) throw Error{"#duplicate-weight"};
						int i = eis.read_int();
						weight = {i};
					} else if( *key == ":index" ) {
						if( index ) throw Error{"#duplicate-index"};
						int i = eis.read_int();
						if( i < 1 || systems.size() < i )
							throw Error{"#index-out-of-range",to_string(i)};
						index = {i};
					} else {
						throw Error{"#unknown-key",*key};
					}
				}
				systems[ index ? *index-1 : 0 ].emplace_back( l, r, weight ? *weight : 1 );
			} else {
				throw Error{"#unknown-command",eis.read_exp()};
			};
			eis.close();
		}
	} else {
		throw Error("#unsupported-format",eis.read_exp());
	}
}

bool Problem::test() {
	{
		auto ifs = ifstream("test.ari");
		auto prob = Problem(ifs);
		for( int i = 0; i < prob.systems.size(); i++ ) {
			cout << "TRS " << i+1 << ":" << endl << prob.systems[i];
		}
	}
	return true;
}
