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

void Problem::_read_format( Exp::Reader& reader ) {
	reader.open();
	reader.read_sym("format");
	_switch( reader.read_sym(), {
		{"TRS",[&](){ _process_trs_format(reader); }},
		{"SRS",[&](){ _process_srs_format(reader); }},
	}, [&](string const& str) {
		throw Error({"#unknown-format",str});
	});
	reader.close();
}

void Problem::_process_trs_format( Exp::Reader& reader ) {
	format = TRS;
	Opt<string> num;
	while( auto key = reader.reads_key() ) {
		_switch( *key, {
			{":number",[&](){
				auto const& new_num = reader.read_sym();
				if( num ) {
					throw Error({"#duplicate-attr",{":number",{*num,new_num}}});
				}
				num = new_num;
			}},
		},[&]( string const& key ){
			throw Error({"#unknown-key",key});
		});
	}
	if( num ) {
		int n = stoi(*num);
		if( n <= 0 || 10 < n ) {
			throw Error({"#out-of-range",{":number",*num}});
		}
		systems = vector<System>(n);
	} else {
		systems = vector<System>(1);
	}
}

void Problem::_process_fun(Exp::Reader& reader) {
	string fun = reader.read_sym();
	Term::Rank rank;
	while( auto key = reader.reads_key() ) {
		_switch( *key, {
			{":arity",[&](){
				rank.set_arity(reader.read_int());
			}},
		},[&]( string const& key ){
			throw Error({"#unknown-key",key});
		});
	}
	reader.close();
	if( !sig.insert(fun,rank) ) {
		throw Error({"#duplicate-fun",fun});
	}
}

void Problem::_process_rule(Term::Reader& reader) {
	Term l = reader.read_term();
	Term r = reader.read_term();
	Opt<string> index;
	while( auto key = reader.reads_key() ) {
		_switch( *key,{
			{":index",[&](){
				index = reader.read_sym();
			}},
		},[&]( string const& key ){
			throw Error({"#unknown-key",key});
		});
	}
	reader.close();
	int i = 0;
	if( index ) {
		i = stoi(*index)-1;
		if( i < 0 || systems.size() <= i ) {
			throw Error({"#index-out-of-range",*index});
		}
	}
	systems[i].push_back({l,r});
}
void Problem::_parse( std::istream& is ) {
	auto reader = Term::Reader(is,sig);
	_read_format(reader);
	while( reader.opens() ) {
		_switch( reader.read_sym(), {
			{"fun",[&](){_process_fun(reader);}},
			{"rule",[&](){_process_rule(reader);}}
		}, [&]( string const& cmd ){
			throw Error({"#unknown-command",cmd});
		});
	}
}
void Problem::write_systems( ostream& os ) {
	for( int i = 0; i < systems.size(); i++ ) {
		cout << "TRS " << i+1 << ":" << endl;
		for( auto const& rule : systems[i] ) {
			cout << '\t' << rule.first << " -> " << rule.second << endl;
		}
	}
}
