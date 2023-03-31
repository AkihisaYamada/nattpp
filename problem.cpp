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
	auto eis = Exp::Reader(is);
	eis.open();
	eis.read_sym("format");
	if( eis.reads_sym("TRS") ) {
		format = TRS;
		Opt<int> num;
		while( auto key = eis.reads_key() ) {
			if( *key == ":number" ) {
				auto const& s = eis.read_sym();
				if( num ) {
					throw Error{"#duplicate-attr",{":number",{*num,s}}};
				}
				num = stoi(s);
				if( *num <= 0 || 10 < *num ) {
					throw Error{"#out-of-range",{":number",*num}};
				}
			} else {
				throw Error{"#unknown-key",*key};
			}
		}
		eis.close();// of format
		systems = vector<TRS::Rules>( num ? *num : 1 );
		auto tis = TRS::Reader(eis,sig);
		while( eis.opens() ) {
			if( eis.reads_sym("fun") ) {
				string fun = eis.read_sym();
				TRS::Rank rank;
				while( auto key = eis.reads_key() ) {
					_switch( *key, {
						{":arity",[&](){
							rank.set_arity(eis.read_int());
						}},
					},[&]( string const& key ){
						throw Error{"#unknown-key",key};
					});
				}
				if( !sig.insert(fun,rank) ) {
					throw Error{"#duplicate-fun",fun};
				}
			} else if( eis.reads_sym("rule") ) {
				auto l = tis.read_term();
				auto r = tis.read_term();
				Opt<string> index;
				while( auto key = eis.reads_key() ) {
					_switch( *key,{
						{":index",[&](){
							index = eis.read_sym();
						}},
					},[&]( string const& key ){
						throw Error{"#unknown-key",key};
					});
				}
				int i = 0;
				if( index ) {
					i = stoi(*index)-1;
					if( i < 0 || systems.size() <= i ) {
						throw Error{"#index-out-of-range",*index};
					}
				}
				systems[i].push_back({l,r});
			} else {
				throw Error{"#unknown-command",eis.read_exp()};
			};
			eis.close();
		}
	} else if( eis.reads_sym("SRS") ) {
		format = SRS;
	}
}

bool Problem::test() {
	auto ifs = fstream("test.ari");
	auto prob = Problem(ifs);
	for( int i = 0; i < prob.systems.size(); i++ ) {
		cout << "TRS " << i+1 << ":" << endl << prob.systems[i];
	}
	return true;
}
