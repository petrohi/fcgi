#ifndef _FcgiHttp_h_
#define _FcgiHttp_h_

#include <map>
#include <boost/array.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/compare.hpp>



namespace fcgi 
{
namespace Http
{
    typedef enum _RequestMethod {
        HTTP_METHOD_ERROR,
        HTTP_METHOD_HEAD,
        HTTP_METHOD_GET,
        HTTP_METHOD_POST,
        HTTP_METHOD_PUT,
        HTTP_METHOD_DELETE,
        HTTP_METHOD_TRACE,
        HTTP_METHOD_OPTIONS,
        HTTP_METHOD_CONNECT
    } RequestMethod;

    typedef enum _EnvParams {
        ePARAM_CONTENT_LENGTH=0,
        ePARAM_CONTENT_TYPE,
        ePARAM_DOCUMENT_ROOT,
        ePARAM_HTTP_ACCEPT,
        ePARAM_HTTP_ACCEPT_CHARSET,
        ePARAM_HTTP_ACCEPT_LANGUAGE,
        ePARAM_HTTP_COOKIE,
        ePARAM_HTTP_HOST,
        ePARAM_HTTP_IF_MODIFIED_SINCE,
        ePARAM_HTTP_IF_NONE_MATCH,
        ePARAM_HTTP_KEEP_ALIVE,
        ePARAM_HTTP_REFERER,
        ePARAM_HTTP_USER_AGENT,
        ePARAM_GATEWAY_INTERFACE,
        ePARAM_PATH_INFO,
        ePARAM_QUERY_STRING,
        ePARAM_REDIRECT_STATUS,
        ePARAM_REMOTE_ADDR,
        ePARAM_REMOTE_PORT,
        ePARAM_REQUEST_METHOD,
        ePARAM_REQUEST_URI,
        ePARAM_SCRIPT_FILENAME,
        ePARAM_SCRIPT_NAME,
        ePARAM_SERVER_ADDR,
        ePARAM_SERVER_NAME,
        ePARAM_SERVER_PORT,
        ePARAM_SERVER_PROTOCOL,
        ePARAM_SERVER_SOFTWARE,
        ePARAM_NUM_MAX,
        ePARAM_UNKNOWN
    } EnvParams;

    class Environment
    {
    public:
        Environment() : _requestMethod(HTTP_METHOD_ERROR) { init(); }

        typedef std::basic_string<char>    String;
        typedef std::map<String, String>   UnknownParamsType;
        typedef boost::array<String, ePARAM_NUM_MAX> KnownParamsType;

        void addParam(const String& name, const String& value)
        {
            EnvParams idx=lookup(name);
            if (idx==ePARAM_UNKNOWN) 
                _uParams.insert(make_pair(name, value));            
            else {
                _kParams[idx]=value;
                switch (idx) {
                case ePARAM_REQUEST_METHOD:
                    setRequestMethod(value);
                    break;
                default:
                    break;
                }
            }
        }

        String getParam(EnvParams idx) const
        {
            return _kParams[idx];
        }

        String getParam(const String& name) const
        {
            EnvParams idx=lookup(name);
            if (idx==ePARAM_UNKNOWN) {
                UnknownParamsType::const_iterator it=_uParams.find(name);
                if (it!=_uParams.end())
                    return it->second;
                else
                    return "";
            }
            else
                return _kParams[idx];
        }

        RequestMethod requestMethod() const {
            return _requestMethod;
        }

        String contentType() const {
            return getParam(ePARAM_CONTENT_TYPE);
        }

        String scriptName() const {
            return getParam(ePARAM_SCRIPT_NAME);
        }

        String pathInfo() const {
            return getParam(ePARAM_PATH_INFO);
        }

    private:
        void setRequestMethod(const String& value) {
            StaticMMapType::const_iterator it(_smMap.find(value));
            if (it!=_smMap.end())
                _requestMethod=it->second;
            else
                _requestMethod=HTTP_METHOD_ERROR;
        }

#if 0
        typedef std::map<String, RequestMethod, boost::function< bool(std::string const &, 
                              std::string const &)> > StaticMMapType;
        typedef std::map<String, EnvParams, boost::function< bool(std::string const &, 
                              std::string const &)> > StaticPMapType;
#endif
        typedef std::map<String, RequestMethod> StaticMMapType;
        typedef std::map<String, EnvParams>     StaticPMapType;

        static StaticPMapType _spMap;
        static StaticMMapType _smMap;

        EnvParams lookup(const std::string& name) const {
            StaticPMapType::const_iterator it=_spMap.find(name);
            if (it==_spMap.end())
                return ePARAM_UNKNOWN;
            else
                return it->second;
        }
        void init();

        KnownParamsType   _kParams; // known
        UnknownParamsType _uParams; // unknown
        RequestMethod     _requestMethod;
    };
}
}
#endif
