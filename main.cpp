#include<map>
#include<fstream>
#include"term.hpp"

using namespace std;

struct SyntaxError : std::exception {
	Exp msg;
	SyntaxError(Exp const&& msg) : msg(msg) {}
};

class Main {
	Dict dict;
	Sig sig;
	vector<pair<Exp,Exp>> trs;
	String const RULE = dict.touch("rule");
	String const FUN = dict.touch("fun");
	String const VAR = dict.touch("var");
public:
	void parse(istream& is) {
		auto reader = ExpReader(is,dict);
		while( reader.opens() ) {
			static map<String,function<void(void)>> _map = {
				{FUN,[&](){
					String fun = reader.read_sym();
					int arity = reader.read_int();
					reader.close();
					sig.insert(fun,arity);
				}},
				{RULE,[&](){
					Exp l = sig.read_term(reader);
					Exp r = sig.read_term(reader);
					reader.close();
					trs.push_back({l,r});
				}}
			};
			auto cmd = reader.read_sym();
			auto fit = _map.find(cmd);
			if( fit == _map.end() ) {
				throw SyntaxError(App{Exp("#unknown-command"),{Exp(cmd)}});
			}
			fit->second();
		}
	}
	void write_trs(ostream& os) {
		for( auto const& rule : trs ) {
			cout << rule.first << " -> " << rule.second << endl;
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
	} catch( Sig::Error e ) {
		cerr << e.e << endl;
	}
}