#ifndef _ApplicationHandler_h_
#define _ApplicationHandler_h_

#include <map>

#include "Http.hpp"

namespace fcgi
{
    class RequestBase;
    
    // just one of a possible application handler implementations.
    class BaseAppHandler
    {
    public:
        BaseAppHandler(RequestBase& base) : _base(base) {}

        // void init() {
        // }

        // called for every FCGI HEADER pair, store it in the environment object
        void params(const std::string& name, const std::string& value) {
            _env.addParam(name, value);
        }

        // called after the last FCGI HEADER pair
        void processParams() {
            _env.processParams();
        }

        void processRequest()
        {
            std::cout << "App::Process"<<std::endl;
            _base.outstream() << "Status: 200\r\n"
                "Content-Type: application/json; charset=utf-8\r\n\r\n";
            for (int i=0; i<Http::PARAM_NUM_MAX; ++i) {
                _base.outstream()<<"PARAM "
                                 <<i<<" "
                                 <<_env.getParam((Http::EnvParams)i)<<"\r\n";
            }

            //_base.outstream()<<"{test=\"test\", id=\""<<i<<"\"}\r\n";
            // _base.errstream()<<"debug cycle"<<i<<"\n";
            //    // std::cout<<i<<std::endl;
            
            _base.errstream() << "Error test\r\n\r\n" << std::endl;
            _base.requestComplete(0);
        }

        // size=0 means end of post data, ready to execute an request
        void postData(const char* data, size_t size)
        {
            if (_env.requestMethod()==Http::HTTP_METHOD_POST &&
                (_env.contentType()=="application/x-www-form-urlencoded")) {
                _env.addPostData(data, size);
            }
        }

        void data(const char* data, size_t size)
        {
            // ignore FCGI_DATA stream
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
