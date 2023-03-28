#ifndef _PROC_HPP
#define _PROC_HPP

#include<unistd.h>
#include<string>
#include<vector>
#include<iostream>
#include<ext/stdio_filebuf.h>

class Proc {
public:
	struct Error : std::exception {
		std::string message;
		Error( std::string const& message ) : message(message) {}
	};
private:
	std::istream _is;
	std::ostream _os;
	pid_t _pid;
	__gnu_cxx::stdio_filebuf<char> _to_filebuf, _from_filebuf;
	Proc( pid_t pid, int to, int from ) :
		_pid(pid),
		_to_filebuf(to,std::ios::out),
		_os(&_to_filebuf),
		_from_filebuf(from,std::ios::in),
		_is(&_from_filebuf) {}
public:
	static Proc make( std::string const& cmd, std::vector<std::string> const& args );
	int from_fd() {
		return _from_filebuf.fd();
	}
	template<typename T>
	Proc& operator<<( T const& x ) {
		_os << x;
		return *this;
	}
	template<typename T>
	Proc& operator>>( T& x ) {
		_is >> x;
		return *this;
	}
};


#endif