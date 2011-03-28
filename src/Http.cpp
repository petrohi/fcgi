#include <stdint.h>
#include <iostream>
#include <sstream>
#include <boost/archive/detail/utf8_codecvt_facet.hpp>

#include "Http.hpp"

using namespace std;

namespace fcgi
{
namespace Http
{
    Environment::StaticPMapType Environment::_spMap;
    Environment::StaticMMapType Environment::_smMap;

    void Environment::init()
    {
        if (!_spMap.empty())
            return; 

        // inserts are presorted for rb-tree, names (enum) are presorted too.
        _spMap.insert(make_pair("GATEWAY_INTERFACE", PARAM_GATEWAY_INTERFACE)); //13

        _spMap.insert(make_pair("HTTP_COOKIE",     PARAM_HTTP_COOKIE));         //6
        _spMap.insert(make_pair("REQUEST_URI",     PARAM_REQUEST_URI));         //20

        _spMap.insert(make_pair("DOCUMENT_ROOT",   PARAM_DOCUMENT_ROOT));       //2
        _spMap.insert(make_pair("HTTP_REFERER",    PARAM_HTTP_REFERER));        //10
        _spMap.insert(make_pair("REDIRECT_STATUS", PARAM_REDIRECT_STATUS));     //16
        _spMap.insert(make_pair("SERVER_NAME",     PARAM_SERVER_NAME));         //24

        _spMap.insert(make_pair("CONTENT_LENGTH",  PARAM_CONTENT_LENGTH));          //1
        _spMap.insert(make_pair("HTTP_ACCEPT_CHARSET", PARAM_HTTP_ACCEPT_CHARSET)); //4
        _spMap.insert(make_pair("HTTP_IF_NONE_MATCH", PARAM_HTTP_IF_NONE_MATCH));   //8
        _spMap.insert(make_pair("HTTP_USER_AGENT", PARAM_HTTP_USER_AGENT));         //12
        _spMap.insert(make_pair("QUERY_STRING",    PARAM_QUERY_STRING));            //15
        _spMap.insert(make_pair("REMOTE_ADDR",     PARAM_REMOTE_ADDR));             //18
        _spMap.insert(make_pair("SCRIPT_NAME",     PARAM_SCRIPT_NAME));             //22
        _spMap.insert(make_pair("SERVER_PROTOCOL", PARAM_SERVER_PROTOCOL));         //26

        _spMap.insert(make_pair("CONTENT_TYPE",    PARAM_CONTENT_TYPE));                 //0
        _spMap.insert(make_pair("HTTP_ACCEPT",     PARAM_HTTP_ACCEPT));                  //3
        _spMap.insert(make_pair("HTTP_ACCEPT_LANGUAGE", PARAM_HTTP_ACCEPT_LANGUAGE));    //5
        _spMap.insert(make_pair("HTTP_IF_MODIFIED_SINCE", PARAM_HTTP_IF_MODIFIED_SINCE));//7
        _spMap.insert(make_pair("HTTP_KEEP_ALIVE", PARAM_HTTP_KEEP_ALIVE));              //9
        _spMap.insert(make_pair("HTTP_HOST",       PARAM_HTTP_HOST));                    //11
        _spMap.insert(make_pair("PATH_INFO",       PARAM_PATH_INFO));                    //14
        _spMap.insert(make_pair("REMOTE_PORT",     PARAM_REMOTE_PORT));                  //17
        _spMap.insert(make_pair("REQUEST_METHOD",  PARAM_REQUEST_METHOD));               //19
        _spMap.insert(make_pair("SCRIPT_FILENAME", PARAM_SCRIPT_FILENAME));              //21
        _spMap.insert(make_pair("SERVER_ADDR",     PARAM_SERVER_ADDR));                  //23
        _spMap.insert(make_pair("SERVER_PORT",     PARAM_SERVER_PORT));                  //25
        _spMap.insert(make_pair("SERVER_SOFTWARE", PARAM_SERVER_SOFTWARE));              //27

        // _smMap.insert(make_pair("ERROR", HTTP_METHOD_ERROR));
        _smMap.insert(make_pair("HEAD",    HTTP_METHOD_HEAD));
        _smMap.insert(make_pair("GET",     HTTP_METHOD_GET));
        _smMap.insert(make_pair("POST",    HTTP_METHOD_POST));
        _smMap.insert(make_pair("PUT",     HTTP_METHOD_PUT));
        _smMap.insert(make_pair("DELETE",  HTTP_METHOD_DELETE));
        _smMap.insert(make_pair("TRACE",   HTTP_METHOD_TRACE));
        _smMap.insert(make_pair("OPTIONS", HTTP_METHOD_OPTIONS));
        _smMap.insert(make_pair("CONNECT", HTTP_METHOD_CONNECT));
    }

    void Environment::addParam(const String& name, const String& value)
    {
        EnvParams idx=lookup(name);
        if (idx==PARAM_UNKNOWN) 
            _uParams.insert(make_pair(name, value));            
        else {
            _kParams[idx]=value;
        }
    }

    void Environment::processParams()
    {
        setRequestMethod(getParam(PARAM_REQUEST_METHOD));
        parseURI(getParam(PARAM_QUERY_STRING).begin(),
                 getParam(PARAM_QUERY_STRING).end(),
                 inserter(_getRequest, _getRequest.begin()));
        std::istringstream(getParam(PARAM_CONTENT_LENGTH)) >> _contentLength;
    }

    void Environment::addPostData(const char* data, size_t size)
    {
        _postBuffer.append(data, size);
        if (size>0) {
            String::const_reverse_iterator rit(find(_postBuffer.rbegin(), _postBuffer.rend(), '&'));
            if (rit!=_postBuffer.rend()) {
                String::const_iterator begin(_postBuffer.begin());
                String::const_iterator end(rit.base());
                parseURI(begin, end, inserter(_postRequest, _postRequest.begin()));
                _postBuffer.erase(0, end-begin);
            }
        }
        else {
            parseURI(_postBuffer.begin(), _postBuffer.end(),
                     inserter(_postRequest, _postRequest.begin()));
            _postBuffer.resize(0);
        }
    }

    template<>
    void HttpString<wchar_t>::decode(const char* data, size_t size, std::basic_string<wchar_t>& string)
    {
        const size_t bufferSize = 512;
        wchar_t buffer[bufferSize];

        if (size)
        {
            codecvt_base::result result = codecvt_base::partial;

            while (result == codecvt_base::partial)
            {
                wchar_t* bufferIt;
                const char* dataIt;

                mbstate_t conversionState = mbstate_t();
                result = use_facet<codecvt<wchar_t, char, mbstate_t> >
                    (locale(locale::classic(), new boost::archive::detail::utf8_codecvt_facet))
                    .in(conversionState, data, data + size, dataIt, buffer, buffer + bufferSize, bufferIt);

                string.append(buffer, bufferIt);
                size -= (dataIt - data);
                data = dataIt;
            }

            if (result == codecvt_base::error)
            {
            }
        }
    }

    template<> template<>
    void HttpString<wchar_t>::encode(const basic_string<wchar_t>& src, ostream& dst)
    {
        const size_t bufferSize = 512;
        char buffer[bufferSize];
        size_t size = src.size();

        ostream_iterator<char> out(dst);

        if (size)
        {
            codecvt_base::result result = codecvt_base::partial;
            char* bufferIt;
            const wchar_t* stringBegin = src.c_str();
            const wchar_t* stringIt;

            while (result == codecvt_base::partial)
            {
                mbstate_t conversionState = mbstate_t();
                result = use_facet<codecvt<wchar_t, char, mbstate_t> >
                    (locale(locale::classic(), new boost::archive::detail::utf8_codecvt_facet))
                    .out(conversionState, stringBegin, stringBegin+size, stringIt, buffer,
                         buffer+bufferSize, bufferIt);

                copy(buffer, bufferIt, out);
                size -= (stringIt - stringBegin);
                stringBegin = stringIt;
            }

            if (result == codecvt_base::error)
            {
            }
        }
    }

    template<> template<>
    void HttpString<wchar_t>::encode(const basic_string<wchar_t>& src, std::string& dst)
    {
        ostringstream out;
        encode(src, static_cast< ostream& > (out) );
        dst=out.str();
    }

#if 0
    template<class In, class Out>
    Out base64Decode(In start, In end, Out destination)
    {
        Out dest=destination;
        for (int buffer, bitPos=-8, padStart; start!=end || bitPos>-6; ++dest) {
            if (bitPos==-8) {
                bitPos=18;
                padStart=-6;
                buffer=0;
                while (bitPos!=-6) {
                    if (start==end)
                        return destination;
                    int value=*start++;
                    if (value >= 'A' && 'Z' >= value)
                        value -= 'A';
                    else if (value >= 'a' && 'z' >= value)
                        value -= 'a' - 26;
                    else if (value >= '0' && '9' >= value)
                        value -= '0' - 52;
                    else if (value == '+')
                        value = 62;
                    else if (value == '/')
                        value = 63;
                    else if (value == '=')
                        { padStart+=8; value=0; }
                    else
                        return destination;
                    buffer |= value << bitPos;
                    bitPos-=6;
                }
                bitPos=16;
            }
            if (padStart==bitPos)
                return ++dest;
            *destination++ = (buffer >> bitPos) & 0xff;
            bitPos-=8;
        }
        return dest;
    }

    template<class In, class Out>
    void base64Encode(In start, In end, Out destination)
    {
        for (int buffer, bitPos=-6, padded; start!=end || bitPos>-6; ++destination) {
            if (bitPos==-6) {
                bitPos=16;
                buffer=0;
                padded=-6;
                while (bitPos!=-8) {
                    if (start!=end) 
                        buffer |= (uint32_t)*(unsigned char*)start++ << bitPos;
                    else
                        padded+=6;
                    bitPos-=8;
                }
                bitPos=18;
            }
            if (padded == bitPos) {
                *destination='=';
                padded-=6;
            }
            else
                *destination=base64Characters[ (buffer >> bitPos)&0x3f ];
            bitPos -= 6;
        }
    }
#endif
}
}
