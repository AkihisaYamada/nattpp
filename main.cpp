#include<map>
#include<fstream>
#include<fcntl.h>
#include"term.hpp"
#include"proc.hpp"

using namespace std;

struct SyntaxError : Exp::Error {
	using Exp::Error::Error;
};

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
enum class Format {
	TRS,
	SRS,
};

class Main {
	Term::Sig sig;
	struct System : vector<pair<Term,Term>> {};
	vector<System> systems;
	Format format;
	/**
	 * @brief Reads (format ...) expression.
	 */
	void _read_format( Exp::Reader& reader ) {
		reader.open();
		reader.read_sym("format");
		_switch( reader.read_sym(), {
			{"TRS",[&](){ _process_trs_format(reader); }},
			{"SRS",[&](){ _process_srs_format(reader); }},
		}, [&](string const& str) {
			throw SyntaxError({"#unknown-format",str});
		});
		reader.close();
	}
	void _process_trs_format( Exp::Reader& reader ) {
		format = Format::TRS;
		Opt<string> num;
		while( auto key = reader.reads_key() ) {
			_switch( *key, {
				{":number",[&](){
					auto const& new_num = reader.read_sym();
					if( num ) {
						throw SyntaxError({"#duplicate-attr",{":number",{*num,new_num}}});
					}
					num = new_num;
				}},
			},[&]( string const& key ){
				throw SyntaxError({"#unknown-key",key});
			});
		}
		if( num ) {
			int n = stoi(*num);
			if( n <= 0 || 10 < n ) {
				throw SyntaxError({"#out-of-range",{":number",*num}});
			}
			systems = vector<System>(n);
		} else {
			systems = vector<System>(1);
		}
	}
	void _process_srs_format( Exp::Reader& reader ) {
		format = Format::SRS;
	}
	void _process_fun(Exp::Reader& reader) {
		string fun = reader.read_sym();
		Term::Rank rank;
		while( auto key = reader.reads_key() ) {
			_switch( *key, {
				{":arity",[&](){
					rank.set_arity(reader.read_int());
				}},
			},[&]( string const& key ){
				throw SyntaxError({"#unknown-key",key});
			});
		}
		reader.close();
		if( !sig.insert(fun,rank) ) {
			throw SyntaxError({"#duplicate-fun",fun});
		}
	}
	void _process_rule(Term::Reader& reader) {
		Term l = reader.read_term();
		Term r = reader.read_term();
		Opt<string> index;
		while( auto key = reader.reads_key() ) {
			_switch( *key,{
				{":index",[&](){
					index = reader.read_sym();
				}},
			},[&]( string const& key ){
				throw SyntaxError({"#unknown-key",key});
			});
		}
		reader.close();
		int i = 0;
		if( index ) {
			i = stoi(*index)-1;
			if( i < 0 || systems.size() <= i ) {
				throw SyntaxError({"#index-out-of-range",*index});
			}
		}
		systems[i].push_back({l,r});
	}
public:
	void parse( istream& is ) {
		auto reader = Term::Reader(is,sig);
		_read_format(reader);
		while( reader.opens() ) {
			_switch( reader.read_sym(), {
				{"fun",[&](){_process_fun(reader);}},
				{"rule",[&](){_process_rule(reader);}}
			}, [&]( string const& cmd ){
				throw SyntaxError({"#unknown-command",cmd});
			});
		}
	}
	void write_systems( ostream& os ) {
		for( int i = 0; i < systems.size(); i++ ) {
			cout << "TRS " << i+1 << ":" << endl;
			for( auto const& rule : systems[i] ) {
				cout << '\t' << rule.first << " -> " << rule.second << endl;
			}
		}
	}
};

int main( int argc, char** argv ) {
	try {
		Main obj;
		istream* pis;
		bool exit_on_error = false;
		if( argc == 1 ) {
			pis = &cin;
		} else {
			pis = new fstream(argv[1]);
			exit_on_error = true;
		}
		obj.parse(*pis);
		obj.write_systems(cout);
	} catch( Term::Reader::Error const& e ) {
		cerr << e.msg << endl;
	} catch( Exp::Error const& e ) {
		cerr << e.msg << endl;
	}
}