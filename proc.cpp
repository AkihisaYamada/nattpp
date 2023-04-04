#include<iostream>
#include"proc.hpp"

using namespace std;

Proc::_Maker::_Maker( string const& cmd, vector<string> const& args ) {
	int to_pipe[2], from_pipe[2];
	if( pipe(to_pipe) || pipe(from_pipe) ) {
		throw Error("pipe");
	}
	pid = fork();
	if( pid == 0 ) {
		char const* argv[args.size()+1];
		size_t i = 0;
		for( ; i < args.size(); i++ ) {
			argv[i] = args[i].c_str();
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
	cerr << "Proc: " << cmd << "; pid=" << pid << endl;
}

Proc::Proc( _Maker const& maker, Opt<ostream&> tee ) :
	_pid(maker.pid),
	_to_filebuf(maker.to,std::ios::out),
	_from_filebuf(maker.from,std::ios::in),
	// this part is tricky.
	_tee(),
	to( tee ? (streambuf*)&_tee.emplace(_to_filebuf,*tee) : &_to_filebuf ),
	from(&_from_filebuf) {
}

void Proc::test() {
	auto cout2 = ofstream("/dev/stdout");
	auto teebuf = TeeBuf(*cout2.rdbuf(),cout);
	auto teeos = ostream(&teebuf);
	teeos << "duplicate me" << endl;
	Proc cat("cat",{"cat","/dev/stdin"});
	cat.to<<"hello world"<<endl;
	cat.finish();
	for( char c; (c=cat.from.get()) != EOF; cout.put(c) ) ;
	Proc teed("cat",{"cat","/dev/stdin"},cout);
	teed.to<<"tee me"<<endl;
	teed.finish();
	for( char c; (c=teed.from.get()) != EOF; cout.put(c) ) ;
}