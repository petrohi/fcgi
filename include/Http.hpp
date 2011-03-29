#ifndef _FcgiHttp_h_
#define _FcgiHttp_h_

#include <stdint.h>
#include <map>
#include <boost/array.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/compare.hpp>
#include <boost/lexical_cast.hpp>

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
        PARAM_CONTENT_LENGTH=0,
        PARAM_CONTENT_TYPE,
        PARAM_DOCUMENT_ROOT,
        PARAM_HTTP_ACCEPT,
        PARAM_HTTP_ACCEPT_CHARSET,
        PARAM_HTTP_ACCEPT_LANGUAGE,
        PARAM_HTTP_COOKIE,
        PARAM_HTTP_HOST,
        PARAM_HTTP_IF_MODIFIED_SINCE,
        PARAM_HTTP_IF_NONE_MATCH,
        PARAM_HTTP_KEEP_ALIVE,
        PARAM_HTTP_REFERER,
        PARAM_HTTP_USER_AGENT,
        PARAM_GATEWAY_INTERFACE,
        PARAM_PATH_INFO,
        PARAM_QUERY_STRING,
        PARAM_REDIRECT_STATUS,
        PARAM_REMOTE_ADDR,
        PARAM_REMOTE_PORT,
        PARAM_REQUEST_METHOD,
        PARAM_REQUEST_URI,
        PARAM_SCRIPT_FILENAME,
        PARAM_SCRIPT_NAME,
        PARAM_SERVER_ADDR,
        PARAM_SERVER_NAME,
        PARAM_SERVER_PORT,
        PARAM_SERVER_PROTOCOL,
        PARAM_SERVER_SOFTWARE,
        PARAM_NUM_MAX,
        PARAM_UNKNOWN
    } EnvParams;

    template<typename CharT>
    struct HttpString
    {
        static void decode(const char* data, size_t size, std::basic_string<CharT>& string);
        static void decode(const std::string& data,       std::basic_string<CharT>& string)
        {
            decode(data.c_str(), data.size(), string);
        }

        template<typename OutputT>
        static void encode(const std::basic_string<CharT>& string, OutputT & data);
    };

    class Environment
    {
    public:
        Environment() :
            _requestMethod(HTTP_METHOD_ERROR),
            _contentLength(0)
        { init(); }

        typedef std::basic_string<char>    String;
        typedef std::map<String, String>   UnknownParamsType;
        typedef boost::array<String, PARAM_NUM_MAX> KnownParamsType;

        void addParam(const String& name, const String& value);
        void processParams();

        void addPostData(const char* data, size_t size);

        String getParam(EnvParams idx) const
        {
            return _kParams[idx];
        }

        std::basic_string<wchar_t> wgetParam(EnvParams idx) const
        {
            std::basic_string<wchar_t> result;
            HttpString<wchar_t>::decode(_kParams[idx], result);
            return result;
        }

        String getParam(const String& name) const
        {
            EnvParams idx=lookup(name);
            if (idx==PARAM_UNKNOWN) {
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
            return _contentType;
        }

        String scriptName() const {
            return getParam(PARAM_SCRIPT_NAME);
        }

        String pathInfo() const {
            return getParam(PARAM_PATH_INFO);
        }

        uint64_t contentLength() const
        {
            return _contentLength;
        }

        // template<typename KeyT, typename ValueT>
        // bool requestVarGet(const KeyT key, ValueT& value) const;

        template<typename ValueT>
        bool requestVarGet(const String &key, ValueT& value) const
        {
            std::map<String, String>::const_iterator it(_getRequest.find(key));
            if (it==_getRequest.end())
                return false;
            value = boost::lexical_cast<ValueT>(it->second);
            return true;
        }

        template<typename ValueT>
        bool requestVarGet(const char* keyp, ValueT& value) const
        {
            std::string key(keyp);
            return requestVarGet<ValueT>(key, value);
        }

        template<typename ValueT>
        bool requestVarGet(const std::wstring &wkey, ValueT& value) const
        {
            std::string key;
            HttpString<wchar_t>::encode(wkey, key);
            return requestVarGet<ValueT>(key, value);
        }

        template<typename ValueT>
        bool requestVarGet(const wchar_t* keyp, ValueT& value) const
        {
            std::wstring key(keyp);
            return requestVarGet<ValueT>(key, value);
        }

        bool requestVarPost(const String& key, String& value) const
        {
            std::map<String, String>::const_iterator it(_postRequest.find(key));
            if (it==_postRequest.end())
                return false;
            value=it->second;
            return true;
        }

    private:
        void setRequestMethod(const String& value) {
            StaticMMapType::const_iterator it(_smMap.find(value));
            if (it!=_smMap.end())
                _requestMethod=it->second;
            else
                _requestMethod=HTTP_METHOD_ERROR;
        }

        typedef std::map<String, RequestMethod> StaticMMapType;
        typedef std::map<String, EnvParams>     StaticPMapType;

        static StaticPMapType _spMap;
        static StaticMMapType _smMap;

        EnvParams lookup(const std::string& name) const {
            StaticPMapType::const_iterator it=_spMap.find(name);
            if (it==_spMap.end())
                return PARAM_UNKNOWN;
            else
                return it->second;
        }

        void init();

        KnownParamsType   _kParams; // known
        UnknownParamsType _uParams; // unknown
        RequestMethod     _requestMethod;
        String            _contentType;
        String            _postBuffer;
        uint64_t          _contentLength;

        std::map<String, String>  _getRequest;
        std::map<String, String> _postRequest;
    };

    template<typename InputT, typename OutputT>
    void decodeXwwwUrlEncoding(InputT src, InputT end, OutputT out)
    {
        while (src!=end) {
            if (*src == '%') {
                char dst(0);
                ++src;
                for(int shift=4; shift>=0 && src!=end; shift-=4) {
                    // 0x41-'A', 0x46-'F'
                    // 0x61-'a', 0x66-'f'
                    if ((*src|0x20)>0x60 && (*src|0x20)<0x67)
                        dst|=((*src|0x20)-0x57)<<shift;
                    else if (*src >= '0' && *src <= '9')
                        dst|=(*src&0x0f)<<shift;
                    ++src;
                }
                *out++=dst;
            }
            else {
                *out++=*src++;
            }
        }
    }

    template<typename InputT, typename OutputT>
    void parseURI(InputT begin, InputT end, OutputT out)
    {
        InputT it(begin);
        while (it!=end) {
            InputT it1(find(it, end, '='));
            if (it1==end)
                break;
            std::string name;
            name.reserve(it1-it+1);

            InputT it2(find(it1, end, '&'));
            std::string value;
            value.reserve(end-it2);

            Http::decodeXwwwUrlEncoding(it, it1, back_inserter(name));
            Http::decodeXwwwUrlEncoding(++it1, it2, back_inserter(value));
            it=++it2;

            out=make_pair(name, value);
        }
    }

    template<>
    inline bool Environment::requestVarGet(const String &key, std::wstring& wvalue) const
    {
        std::map<String, String>::const_iterator it(_getRequest.find(key));
        if (it==_getRequest.end())
            return false;
        HttpString<wchar_t>::decode(it->second, wvalue);
        return true;
    }

    template<>
    inline bool Environment::requestVarGet(const String &key, String& value) const
    {
        std::map<String, String>::const_iterator it(_getRequest.find(key));
        if (it==_getRequest.end())
            return false;
        value = it->second;
        return true;
    }

}} // fcgi::Http

#endif
