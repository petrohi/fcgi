#ifndef _ApplicationHandler_h_
#define _ApplicationHandler_h_

#include <map>

namespace fcgi
{
    class RequestBase;
    class BaseAppHandler
    {
    public:
        BaseAppHandler(RequestBase& base) : _base(base) {}

        void init() {
        }

        // size=0 means end of params
        void params(const std::string& name, const std::string& value) {
            std::cout << "App::PARAMS "<<name<<" "<<value<<std::endl;
            _params.insert(make_pair(name, value));
        }

        void processParams() {
        }

        void processRequest() {
            std::cout << "App::Process"<<std::endl;
            _base.outstream() << "Status: 200\r\n"
                "Content-Type: application/json; charset=utf-8\r\n\r\n";
            _base.errstream() << "Error test\r\n\r\n" << std::endl;
            _base.requestComplete(0);
        }

        // size=0 means end of post data, execute request
        void postData(const char* data, size_t size)
        {
            std::cout << "App::IN"<<std::endl;
        }

        void data(const char* data, size_t size)
        {
            std::cout << "App::DATA"<<std::endl;
        }

        // WebServer requests to abort a request
        void abort() {
            std::cout << "App::ABORT"<<std::endl;
            _base.requestComplete(0);
        }

        RequestBase& _base;

        std::map<std::string, std::string> _params;
    };
}

#endif
