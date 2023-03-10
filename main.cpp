#include<map>
#include<fstream>
#include"term.hpp"

using namespace std;

struct SyntaxError : std::exception {
	Exp msg;
	SyntaxError(Exp const&& msg) : msg(msg) {}
};

static void _switch(
	String const& key,
	map<String,function<void(void)>> map,
	function<void(String const&)> other
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

class Main {
	Dict dict;
	Sig sig;
	vector<vector<pair<Exp,Exp>>> trs;
	String const FORMAT = dict.touch("format");
	String const TRS = dict.touch("TRS");
	String const SRS = dict.touch("SRS");
	String const RULE = dict.touch("rule");
	String const FUN = dict.touch("fun");
	String const VAR = dict.touch("var");
	String const NUMBER_KEY = dict.touch(":number");
	String const INDEX_KEY = dict.touch(":index");
	String const ARITY_KEY = dict.touch(":arity");
	/**
	 * @brief Reads (format ...) expression.
	 */
	void _read_format(ExpReader& reader) {
		reader.open();
		reader.read_sym(FORMAT);
		bool srs;
		_switch( reader.read_sym(), {
			{TRS,[&](){
				srs = false;
				optional<String> num;
				while( auto key = reader.reads_key() ) {
					_switch( *key, {
						{NUMBER_KEY,[&](){
							num = reader.read_sym();
						}},
					},[&](String const& key){
						throw SyntaxError(App{UNKNOWN_KEY,{key}});
					});
				}
				if( num ) {
					int n = stoi(*num);
					if( n <= 0 || 10 < n ) {
						throw SyntaxError(App{OUT_OF_RANGE,{NUMBER_KEY,*num}});
					}
					trs = vector<vector<pair<Exp,Exp>>>(n);
				} else {
					trs = vector<vector<pair<Exp,Exp>>>(1);
				}
			}},
			{SRS,[&](){
				srs = true;
			}},
		}, [&](String const& str) {
			throw SyntaxError(App{UNKNOWN_FORMAT,{str}});
		});
		reader.close();
	}
public:
	void parse(istream& is) {
		auto reader = ExpReader(is,dict);
		_read_format(reader);
		while( reader.opens() ) {
			_switch( reader.read_sym(), {
				{FUN,[&](){
					String fun = reader.read_sym();
					FunInfo info;
					while( auto key = reader.reads_key() ) {
						_switch( *key, {
							{ARITY_KEY,[&](){
								info.set_arity(reader.read_int());
							}},
						},[&](String const& key){
							throw SyntaxError(App{UNKNOWN_KEY,{key}});
						});
					}
					reader.close();
					sig.insert(fun,info);
				}},
				{RULE,[&](){
					Exp l = sig.read_term(reader);
					Exp r = sig.read_term(reader);
					optional<String> index;
					while( auto key = reader.reads_key() ) {
						_switch( *key,{
							{INDEX_KEY,[&](){
								index = reader.read_sym();
							}},
						},[&](String const& key){
							throw SyntaxError(App{UNKNOWN_KEY,{key}});
						});
					}
					reader.close();
					int i = 0;
					if( index ) {
						i = stoi(*index)-1;
						if( i < 0 || trs.size() <= i ) {
							throw SyntaxError(App{OUT_OF_RANGE,{INDEX_KEY,*index}});
						}
					}
					trs[i].push_back({l,r});
				}}
			}, [&](String const& cmd){
				throw SyntaxError(App{UNKNOWN_CMD,{cmd}});
			});
		}
	}
	void write_trs(ostream& os) {
		for( int i = 0; i < trs.size(); i++ ) {
			cout << "TRS " << i+1 << ":" << endl;
			for( auto const& rule : trs[i] ) {
				cout << '\t' << rule.first << " -> " << rule.second << endl;
			}
		}
	}
};

int main(int argc, char** argv) {
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