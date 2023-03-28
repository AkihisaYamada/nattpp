#include<iostream>
#include"proc.hpp"

using namespace std;

Proc Proc::make( string const& cmd, vector<string> const& args ) {
	int to_pipe[2], from_pipe[2];
	if( pipe(to_pipe) || pipe(from_pipe) ) {
		throw Error("pipe");
	}
	pid_t pid = fork();
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
	return Proc(pid,to_pipe[1],from_pipe[0]);
}
