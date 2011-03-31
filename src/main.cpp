#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <string>
#include <iostream>
#include <hiredis/async.h>

#include <ev++.h>

#include <hiredispp/hiredispp.h>
#include <hiredispp/hiredispp_async.h>

#include <boost/bind.hpp>

#include "Manager.hpp"
#include "Request.hpp"
#include "AppHandler.hpp"

using namespace std;
using namespace hiredispp;
using namespace fcgi;
using namespace fcgi::Http;

class Main
{
public:
    RedisConnectionAsync _ac;

    Main(const string& host, int port) :
        _ac(host, port)
    {
        connect();
    }

    void connect()
    {
        _ac.connect(boost::bind(&Main::onConnected, this),
                    boost::bind(&Main::onDisconnected, this, _1));
    }

    // void setDone(RedisConnectionAsync& ac, void *reply) {
    //    cout << "Main: done" << endl;
    //    _ac.disconnect();
    // }

    void onConnected() {
        _connected=true;
        cout << "connected" << endl;
        // RedisCommandBase<char> cmd("SET");
        // cmd << "mykey1" << "123" ;
        // _ac.execAsyncCommand(cmd, boost::bind(&Main::setDone, this, _1, _2) );
        
    }
    void onDisconnected(int status) {
        _connected=false;
        cout << "Main: disconnected " << status << endl;
    }

    bool _connected;
}
  *__main=NULL;

class App : public BaseAppHandler, public boost::enable_shared_from_this<App>
{
public:
    App(RequestBase& base) : BaseAppHandler(base) {}

    void execDone(RedisConnectionAsync& ac, Redis::Element* reply)
    {
        if (reply) {
            vector<string> v;
            reply->toVector(v);
            copy(v.begin(), v.end(), ostream_iterator<string>(_base.outstream(), " "));
        }
        _base.outstream()<<"done"<<endl;
        _base.requestComplete(-1);
    }

    void processRequest()
    {
        std::cout << "App::Process"<<std::endl;
        _base.outstream() << "Status: 200\r\n"
            "Content-Type: application/json; charset=utf-8\r\n\r\n{ [\n";
        Environment::StaticPMapType::const_iterator it(Http::Environment::paramsMap().begin()),
            end(Http::Environment::paramsMap().end());
        while (it!=end) {
            _base.outstream()<<"{ PARAM_ID="
                             <<it->second<<", name=\""
                             <<it->first<<"\", value=\""
                             <<_env.getParam(it->second)
                             <<"\" }";
            ++it;
            _base.outstream()<<((it==end) ? " " : ",\n" );
        }
        _base.outstream() <<"]\n";
        try {
            if (_env.getParam(PARAM_SCRIPT_NAME) == "/connectRedis") {
                if (__main==NULL) {
                    string host;
                    int    port;
                    if (_env.requestVarGet("host", host) && _env.requestVarGet("port", port)) {
                        _base.outstream() <<"connectRedis "<< host <<":" << port <<"\n";
                        __main = new Main(host,port);
                    }
                }
                else {
                    _base.outstream() <<"connectRedis - connection exists, close it first."
                                      << std::endl;
                }
            }
            else if (_env.getParam(PARAM_SCRIPT_NAME) == "/execRedisCmd") {
                string name;
                if (__main && __main->_connected && _env.requestVarGet("name", name)) {
                    RedisCommandBase<char> cmd(name);
                    
                    _base.outstream() <<"execCmd '"<<name;
                    
                    int i=0; string param;
                    while (true) {
                        ostringstream paramName;
                        paramName << "param" << i++;
                        if (!_env.requestVarGet(paramName.str(), param))
                            break;
                        cmd << param;
                        _base.outstream() << " " << param;
                    }
                    _base.outstream() <<"'\n";

                    map<string,string>::const_iterator it(_env.getRequestMap().begin()),
                        end(_env.getRequestMap().end());
                    while (it!=end) {
                        if (it->first.size()>5 && it->first.substr(0,5) ==  "param")
                            cout << it->first << " "<< it->second << endl;
                        ++it;
                    }

                    __main->_ac.execAsyncCommand(cmd,
                                                 boost::bind(&App::execDone,
                                                             shared_from_this(), _1, _2) );
                    return;
                }
                else {
                    _base.outstream() <<"open connection first."
                                      << std::endl;
                }
            }
        }
        catch (const RedisException& ex) {
            _base.outstream() << "{ RedisException=\""<< ex.what() << "\"} }\n\r\n";
        }
        _base.outstream() << "}\n\r\n";
        _base.requestComplete(0);
    }

private:
    //    string host("127.0.0.1");
    //int port=6379;
    
};

namespace std {
    std::ostream& operator<< (std::ostream& out, const std::pair<std::string, std::string> &p) {
        return out << p.first << ":" << p.second;
    }
}

int main(int argc, char** argv) {

#if 0
    std::string test(argv[1]);
    std::map<string, string> m;
    Http::parseURI(test.begin(), test.end(), inserter(m, m.begin()));
    std::copy(m.begin(), m.end(), std::ostream_iterator< std::pair<std::string, std::string>, char>(cout,"\n") );
    return 0;
#endif


    if (argc==3) {
         if (!strcasecmp(argv[1], "unix")) {
            close(0);
            int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
            if (fd!=0) {
                cout << "can't rebind 0 fd" << endl;
                return 0;
            }
            struct sockaddr_un addr;
            int size=strlen(argv[2]);
            memset(&addr, 0, sizeof(struct sockaddr_un));
            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, argv[2], size+1);//, sizeof(struct sockaddr_un)));
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
    }
    else if (argc!=1) {
        cout << "usage: async path_to_unix_socket" << endl;
        return 0;
    }

    signal(SIGPIPE, SIG_IGN);
    ev_default_loop(0);
    // string host("127.0.0.1");
    // int port=6379;
    // Main m(host, port);
    // __main=&m;

    Manager< class App > fcgi;
    ev_loop(EV_DEFAULT, 0);

    return 0;
}
