#include<map>
#include<fstream>
#include"exp.hpp"

using namespace std;

struct SyntaxError : std::exception {
	Exp msg;
	SyntaxError(Exp const&& msg) : msg(msg) {}
};

class Main {
	Dict dict;
	vector<pair<Exp,Exp>> trs;
	String const RULE = dict.touch("rule");
public:
	void read_trs(istream& is) {
		for(;;) {
			auto e = dict.reads_exp(is);
			if( !e.has_value() ) {
				break;
			}
			auto es = e->app();
			if( !es ) {
				throw SyntaxError({Exp("#malformed-command"),*e});
			}
			auto it = es->begin();
			if( it == es->end() ) {
				throw SyntaxError({Exp("#malformed-command"),*e});
			}
			auto cmd = it->sym();
			if( !cmd ) {
				throw SyntaxError({Exp("#malformed-command"),*e});
			}
			it++;
			static map<String,function<void(void)>> _map = {
				{RULE,[&](){
					if( it == es->end() ) {
						throw SyntaxError({Exp("#too-few-arguments"),*e});
					}
					Exp const& l = *it;
					it++;
					if( it == es->end() ) {
						throw SyntaxError({Exp("#too-few-arguments"),*e});
					}
					Exp const& r = *it;
					it++;
					if( it != es->end() ) {
						throw SyntaxError({Exp("#too-many-arguments"),*e});
					}
					trs.push_back({l,r});
				}}
			};
			auto fit = _map.find(*cmd);
			if(  fit == _map.end() ) {
				throw SyntaxError({Exp("#unknown-command"),*e});
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
	Main obj;
	istream* pis;
	bool exit_on_error = false;
	if( argc == 1 ) {
		pis = &cin;
	} else {
		pis = new fstream(argv[1]);
		exit_on_error = true;
	}
	obj.read_trs(*pis);
	obj.write_trs(cout);
}