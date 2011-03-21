#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <string>
#include <iostream>
#include <hiredis/async.h>

#include <ev++.h>

#include "../hiredispp/hiredispp.h"
#include "../hiredispp/hiredispp_a.h"

#include <boost/bind.hpp>

#include "Manager.hpp"
#include "Request.hpp"
#include "AppHandler.hpp"

using namespace std;
using namespace hiredispp;
using namespace fcgi;

class Main
{
	RedisConnectionAsync _ac;

public:
	Main(const string& host, int port) :
		_ac(host, port)
	{
		_ac.connect(boost::bind(&Main::onConnected, this),
					boost::bind(&Main::onDisconnected, this, _1));
	}

	void setDone(RedisConnectionAsync& ac, void *reply) {
		cout << "done" << endl;
		_ac.disconnect();
	}
	void onConnected() {
		cout << "connected" << endl;
		RedisCommandBase<char> cmd("SET");
		cmd << "mykey1" << "123" ;
		_ac.execAsyncCommand(cmd, boost::bind(&Main::setDone, this, _1, _2) );
	}
	void onDisconnected(int status) {
		cout << "disconnected " << status << endl;
	}
};

class App : public BaseAppHandler
{
public:
	App(RequestBase& base) : BaseAppHandler(base) {}
};

int main(int argc, char** argv) {

	if (argc!=3) {
		cout << "usage: async path_to_unix_socket" << endl;
		return 0;
	}

	if (!strcasecmp(argv[1], "unix")) {
		close(0);
		int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
		if (fd!=0) {
			cout << "can't rebind 0 fd" << endl;
			return 0;
		}
		struct sockaddr_un addr;
		memset(&addr, 0, sizeof(struct sockaddr_un));
		addr.sun_family = AF_UNIX;
		strncpy(addr.sun_path, argv[1], sizeof(addr.sun_path)-1);
		if (bind(fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_un))==-1) {
			perror("bind");
			exit(-1);
		}
		listen(fd,2);
	}
	else {
		int port=atoi(argv[2]);
		close(0);
		int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
		if (fd!=0) {
			cout << "can't rebind 0 fd" << endl;
			return 0;
		}
		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(struct sockaddr_in));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = INADDR_ANY;
		if (bind(fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in))==-1) {
			perror("bind");
			exit(-1);
		}
		listen(fd, 2);
	}

#if 0
	int fd=socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, argv[1], sizeof(addr.sun_path)-1);
	if (connect(fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_un))==-1) {
		perror("connect");
		exit(-1);
	}
	close(0);
	dup(fd);
#endif

    signal(SIGPIPE, SIG_IGN);
	ev_default_loop(0);
	string host("127.0.0.1");
	int port=6379;
	// Main m(host, port);

	Manager< class App > fcgi;

	ev_loop(EV_DEFAULT, 0);
	return 0;
}
