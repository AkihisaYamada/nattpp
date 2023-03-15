#include<map>
#include<fstream>
#include"term.hpp"

using namespace std;

struct SyntaxError : std::exception {
	Exp msg;
	SyntaxError(Exp const&& msg) : msg(msg) {}
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
static Exp const UNKNOWN_FORMAT = Exp("#unknown-format");
static Exp const UNKNOWN_CMD = Exp("#unknown-command");
static Exp const UNKNOWN_KEY = Exp("#unknown-keyword");
static Exp const OUT_OF_RANGE = Exp("#out-of-range");
static Exp const INVALID_ATTRIBUTE = Exp("#invalid-attribute");
static Exp const DUP_FUN_DECL = Exp("#duplicate-fun");

enum class Format {
	TRS,
	SRS,
};

class Main {
	Sig sig;
	vector<vector<pair<Exp,Exp>>> trs;
	Format format;
	/**
	 * @brief Reads (format ...) expression.
	 */
	void _read_format( ExpReader& reader ) {
		reader.open();
		reader.read_sym("format");
		_switch( reader.read_sym(), {
			{"TRS",[&](){ _process_trs_format(reader); }},
			{"SRS",[&](){ _process_srs_format(reader); }},
		}, [&](string const& str) {
			throw SyntaxError(UNKNOWN_FORMAT(str));
		});
		reader.close();
	}
	void _process_trs_format( ExpReader& reader ) {
		format = Format::TRS;
		optional<string> num;
		while( auto key = reader.reads_key() ) {
			_switch( *key, {
				{":number",[&](){
					auto const& new_num = reader.read_sym();
					if( num ) {
						throw SyntaxError(INVALID_ATTRIBUTE(Exp(":number",{*num,new_num})));
					}
					num = new_num;
				}},
			},[&]( string const& key ){
				throw SyntaxError(Exp(UNKNOWN_KEY)(key));
			});
		}
		if( num ) {
			int n = stoi(*num);
			if( n <= 0 || 10 < n ) {
				throw SyntaxError(OUT_OF_RANGE(Exp(":number")(*num)));
			}
			trs = vector<vector<pair<Exp,Exp>>>(n);
		} else {
			trs = vector<vector<pair<Exp,Exp>>>(1);
		}
	}
	void _process_srs_format( ExpReader& reader ) {
		format = Format::SRS;
	}
	void _process_fun(ExpReader& reader) {
		string fun = reader.read_sym();
		FunInfo info;
		while( auto key = reader.reads_key() ) {
			_switch( *key, {
				{":arity",[&](){
					info.set_arity(reader.read_int());
				}},
			},[&]( string const& key ){
				throw SyntaxError(Exp(UNKNOWN_KEY)(key));
			});
		}
		reader.close();
		if( !sig.insert(fun,info) ) {
			throw SyntaxError(DUP_FUN_DECL(fun));
		}
	}
	void _process_rule(ExpReader& reader) {
		Exp l = sig.read_term(reader);
		Exp r = sig.read_term(reader);
		optional<string> index;
		while( auto key = reader.reads_key() ) {
			_switch( *key,{
				{":index",[&](){
					index = reader.read_sym();
				}},
			},[&]( string const& key ){
				throw SyntaxError(Exp(UNKNOWN_KEY)(key));
			});
		}
		reader.close();
		int i = 0;
		if( index ) {
			i = stoi(*index)-1;
			if( i < 0 || trs.size() <= i ) {
				throw SyntaxError(OUT_OF_RANGE(Exp(":index")(*index)));
			}
		}
		trs[i].push_back({l,r});
	}
public:
	void parse( istream& is ) {
		auto reader = ExpReader(is);
		_read_format(reader);
		while( reader.opens() ) {
			_switch( reader.read_sym(), {
				{"fun",[&](){_process_fun(reader);}},
				{"rule",[&](){_process_rule(reader);}}
			}, [&]( string const& cmd ){
				throw SyntaxError(UNKNOWN_CMD(cmd));
			});
		}
	}
	void write_trs( ostream& os ) {
		for( int i = 0; i < trs.size(); i++ ) {
			cout << "TRS " << i+1 << ":" << endl;
			for( auto const& rule : trs[i] ) {
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
		obj.write_trs(cout);
	} catch( Sig::Error const& e ) {
		cerr << e.e << endl;
	} catch( SyntaxError const& e ) {
		cerr << e.msg << endl;
	} catch( ExpError const& e ) {
		cerr << e.msg << endl;
	}
}