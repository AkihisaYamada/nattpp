#ifndef _PROC_HPP
#define _PROC_HPP

#include<unistd.h>
#include<string>
#include<vector>
#include<iostream>
#include<ext/stdio_filebuf.h>
#include"opt.hpp"

class TeeBuf : public std::streambuf {
    std::streambuf& os1;
    std::ostream& os2;
	TeeBuf( TeeBuf const& ) = delete;
public:
	TeeBuf( std::streambuf& os1, std::ostream& os2 ) : os1(os1), os2(os2) {}
private:
	virtual int overflow(int c) {
		os2.put(c);
		return os1.sputc(c);
	}
	virtual int sync() {
		os2.flush();
		return os1.pubsync();
	}
};

class Proc {
public:
	struct Error : std::exception {
		std::string message;
		Error( std::string const& message ) : message(message) {}
	};
private:
	pid_t _pid;
	// structures to make file descriptors into streams. Must be declared before the iostream variables.
	__gnu_cxx::stdio_filebuf<char> _to_filebuf, _from_filebuf;
	Opt<TeeBuf> _tee;
public:
	std::ostream to;
	std::istream from;
private:
	struct _Maker {
		pid_t pid;
		int to;
		int from;
		_Maker( std::string const& cmd, std::vector<std::string> const& args );
	};
	Proc( _Maker const& maker, Opt<std::ostream&> tee );
	Proc( Proc const& other ) = delete;
	Proc& operator=( Proc const& other ) = delete;
public:
	~Proc() {
		std::cerr << "~Proc: pid=" << _pid << std::endl;
	}
	Proc( std::string const& cmd, std::vector<std::string> const& args, Opt<std::ostream&> tee = {} ) :
		Proc(_Maker(cmd,args),tee) {}
	void finish() {
		_to_filebuf.close();
	}
	static void test();
};


#endif