#ifndef _PROC_HPP
#define _PROC_HPP

#include<unistd.h>
#include<string>
#include<vector>
#include<iostream>
#include<ext/stdio_filebuf.h>
#include"exp.hpp"

/** either existing ostream pointer or an ostream */
class OStream {
	std::ostream* const _ptr;
	std::ofstream _ofs;
public:
	OStream( std::ofstream&& ofs ) : _ofs(std::move(ofs)), _ptr(&_ofs) {}
	OStream( std::ostream& other ) : _ptr(&other) {}
	OStream( OStream&& other ) :
		// tricky! If other is holding ofstream, then this _ptr should point to this _ofs.
		_ptr( other._ptr == &other._ofs ? &_ofs : other._ptr ),
		_ofs(std::move(other._ofs))
	{}
	operator std::ostream*() & { return _ptr; }
	std::ostream* operator->() & { return _ptr; }
	static OStream of( Exp const& x );
};

class TeeBuf : public std::streambuf {
    std::streambuf& buf1;
    OStream tee;
public:
	TeeBuf( std::streambuf& buf1, OStream&& tee ) : buf1(buf1), tee(std::move(tee)) {}
private:
	int overflow(int c) override {
		tee->put(c);
		return buf1.sputc(c);
	}
	int sync() override {
		tee->flush();
		return buf1.pubsync();
	}
};

class Proc {
public:
private:
	pid_t _pid;
	// structures to make file descriptors into streams. Must be declared before the iostream variables.
	std::filebuf _to_filebuf, _from_filebuf;
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
	Proc( _Maker const& maker, Opt<OStream>&& tee );
	Proc( Proc const& other ) = delete;
	Proc& operator=( Proc const& other ) = delete;
public:
	~Proc() {
		std::cerr << "~Proc: pid=" << _pid << std::endl;
	}
	Proc( std::string const& cmd, std::vector<std::string> const& args, Opt<OStream>&& tee = {} ) :
		Proc(_Maker(cmd,args),std::move(tee)) {}
	void finish() {
		_to_filebuf.close();
	}
	static void test();
	static Proc of( Exp const& x );
};

#endif