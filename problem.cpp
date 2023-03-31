#include<fstream>
#include"problem.hpp"
#include"srs.hpp"

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

static bool _unknown_key( string_view const& key ) {
	throw Problem::Error{"#unknown-key",key};
}
static bool _process_number( string_view const& key, Exp::Reader& eis, int& num ) {
	if( key != ":number" ) {
		return false;
	}
	auto const& s = eis.read_sym();
	if( num != 0 ) {
		throw Problem::Error{"#duplicate-attr",{":number",{num,s}}};
	}
	num = stoi(s);
	if( num <= 0 || 10 < num ) {
		throw Problem::Error{"#out-of-range",{":number",num}};
	}
	return true;
}
static bool _process_index( Problem& x, string_view const& key, Exp::Reader& eis, int& index ) {
	if( key != ":index" ) {
		return false;
	}
	if( index != 0 ) {
		throw Problem::Error{"#duplicate-index"};
	}
	index = eis.read_int();
	if( index < 1 || x.systems.size() < index ) {
		throw Problem::Error{"#index-out-of-range",index};
	}
	return true;
}

Problem::Problem( istream& is ) {
	auto eis = Exp::Reader(is);
	eis.open();
	eis.read_sym("format");
	if( eis.reads_sym("TRS") ) {
		format = TRS;
		int num = 0;
		while( auto key = eis.reads_key() ) {
			_process_number(*key,eis,num) ||
			_unknown_key(*key);
		}
		eis.close();// of format
		systems = vector<Trs::Rules>( num == 0 ? 1 : num );
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
						arity = eis.read_int();
					} else {
						throw Error{"#unknown-key",*key};
					}
				}
				if( !sig.insert(fun,Trs::Rank{arity}) ) {
					throw Error{"#duplicate-fun",fun};
				}
			} else if( eis.reads_sym("rule") ) {
				auto l = tis.read(), r = tis.read();
				int index = 0;
				while( auto key = eis.reads_key() ) {
					_process_index(*this,*key,eis,index) ||
					_unknown_key(*key);
				}
				systems[ index > 0 ? index-1 : 0 ].push_back({l,r});
			} else {
				throw Error{"#unknown-command",eis.read_exp()};
			};
			eis.close();
		}
	} else if( eis.reads_sym("SRS") ) {
		format = SRS;
		int num = 0;
		while( auto key = eis.reads_key() ) {
			_process_number(*key,eis,num) ||
			_unknown_key(*key);
		}
		eis.close();// of format
		systems = vector<Trs::Rules>( num == 0 ? 1 : num );
		auto sis = Srs::Reader(eis,sig);
		while( eis.opens() ) {
			if( eis.reads_sym("rule") ) {
				auto l = sis.read(), r = sis.read();
				int index = 0;
				while( auto key = eis.reads_key() ) {
					_process_index(*this,*key,eis,index) ||
					_unknown_key(*key);
				}
				systems[ index > 0 ? index-1 : 0 ].push_back({l,r});
			} else {
				throw Error{"#unexpected-command",eis.read_exp()};
			}
			eis.close();
		}
	}
}

bool Problem::test() {
	{
		auto ifs = fstream("test.ari");
		auto prob = Problem(ifs);
		for( int i = 0; i < prob.systems.size(); i++ ) {
			cout << "TRS " << i+1 << ":" << endl << prob.systems[i];
		}
	}
	{
		auto ifs = fstream("srs.ari");
		auto prob = Problem(ifs);
		for( int i = 0; i < prob.systems.size(); i++ ) {
			cout << "SRS " << i+1 << ":" << endl << prob.systems[i];
		}
	}
	return true;
}
