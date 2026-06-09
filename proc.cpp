#include<iostream>
#include"proc.hpp"

using namespace std;

Proc::_Maker::_Maker( char const* cmd, vector<char const*> const& args ) {
	int to_pipe[2], from_pipe[2];
	if( pipe(to_pipe) || pipe(from_pipe) ) {
		throw Error("pipe");
	}
	pid = fork();
	if( pid == 0 ) {
		char const* argv[args.size()+1];
		size_t i = 0;
		for( ; i < args.size(); i++ ) {
			argv[i] = args[i];
		}
		argv[i] = nullptr;
		dup2(to_pipe[0],STDIN_FILENO);
		dup2(from_pipe[1],STDOUT_FILENO);
		close(to_pipe[0]);
		close(to_pipe[1]);
		close(from_pipe[0]);
		close(from_pipe[1]);
		execvp(argv[0],(char**)argv);
		cerr << "execvp failed" << endl;
		exit(-1);
	}
	close(to_pipe[0]);
	close(from_pipe[1]);
	to = to_pipe[1];
	from = from_pipe[0];
//	cerr << "Proc: " << cmd << "; pid=" << pid << endl;
}

Proc::Proc( _Maker const& maker, Opt<OStream>&& tee ) :
	_pid(maker.pid),
	_to_filebuf(__gnu_cxx::stdio_filebuf<char>(maker.to,std::ios::out)),
	_from_filebuf(__gnu_cxx::stdio_filebuf<char>(maker.from,std::ios::in)),
	// this part is tricky.
	_tee( tee ? Opt<TeeBuf>::make(_to_filebuf,*(std::move(tee))) : Opt<TeeBuf>{} ),
	to( _tee ? (std::streambuf*)&*_tee : &_to_filebuf ),
	from(&_from_filebuf) {
}

OStream OStream::of( Exp const& x ) {
	auto const& f = x.fun();
	size_t n = x.args().size();
	if( f == "cout" ) {
		if( n == 0 ) return cout;
	} else if( f == "cerr" ) {
		if( n == 0 ) return cerr;
	} else if( f == "file" ) {
		if( n == 1 )
		if( auto const& path = x.arg(0).unapplied() ) {
			return ofstream(*path);
		}
	}
	throw Error("#malformed-out",x);
};

Proc Proc::of( Exp const& x ) {
	auto const& f = x.fun();
	if( f == "cmd" ) {
		size_t n = 0;
		auto const& cmd = x.get_arg(n).unapplied().value_or_throw( Error("#malformed-cmd",x) ).c_str();
		vector<char const*> cmd_args;
		while( auto const& arg = x.gets_arg(n) ) {
			auto str = arg->unapplied().value_or_throw( Error("#malformed-cmd-arg",*arg) );
			cmd_args.push_back(str.c_str());
		}
		Opt<OStream> tee;
		x.process_keys(n,[&]( auto key, auto val ){
			if( key == "tee" ) {
				tee.emplace(OStream::of(val));
				return true;
			}
			return false;
		});
		return Proc(cmd,cmd_args,std::move(tee));
	}
	throw Error("#malformed-proc",x);
}

void Proc::test() {
	cout << "=== Proc test ===" << endl;
	auto cout2 = ofstream("/dev/stdout");
	auto teebuf = TeeBuf(*cout2.rdbuf(),cout);
	auto teeos = ostream(&teebuf);
	teeos << "duplicate me" << endl;
	Proc cat("cat",{"cat","/dev/stdin"});
	cat.to<<"hello world"<<endl;
	cat.finish();
	for( char c; (c=cat.from.get()) != EOF; cout.put(c) ) ;
	Proc teed("cat",{"cat","/dev/stdin"},{cout});
	teed.to<<"tee me"<<endl;
	teed.finish();
	for( char c; (c=teed.from.get()) != EOF; cout.put(c) ) ;
}