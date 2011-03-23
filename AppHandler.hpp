#ifndef _ApplicationHandler_h_
#define _ApplicationHandler_h_

#include <map>

#include "Http.hpp"

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
            std::cout << name<<" "<<value<<std::endl;
            _env.addParam(name, value);
        }

        void processParams() {
        }

        void processRequest() {
            std::cout << "App::Process"<<std::endl;
            _base.outstream() << "Status: 200\r\n"
                "Content-Type: application/json; charset=utf-8\r\n\r\n";

            for (int i=0; i<10; ++i) {
                _base.outstream()<<"{test=\"test\", id=\""<<i<<"\"}\r\n";
                // _base.errstream()<<"debug cycle"<<i<<"\n";
            //    // std::cout<<i<<std::endl;
            }

            _base.errstream() << "Error test\r\n\r\n" << std::endl;
            _base.requestComplete(0);
        }

        // size=0 means end of post data, execute request
        void postData(const char* data, size_t size)
        {
            std::cout << "App::IN"<<std::endl;
            _env.addPostData(data, size);
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

        void handleError(const Exceptions::FcgiException& ex) {
        }

        RequestBase& _base;

        Http::Environment _env;
    };

}

#endif
