#include <stdint.h>
#include <iostream>
#include <sstream>
#include "Http.hpp"

using namespace std;

namespace fcgi {
namespace Http
{
#if 0
    bool str_iless(const std::basic_string<char> & a,
                   const std::basic_string<char> & b)
    {
        return boost::algorithm::lexicographical_compare(a, b,
                                                         boost::algorithm::is_iless());
    }

    Environment::StaticPMapType Environment::_spMap(&str_iless);
    Environment::StaticMMapType Environment::_smMap(&str_iless);
#endif

    Environment::StaticPMapType Environment::_spMap;
    Environment::StaticMMapType Environment::_smMap;

    void Environment::init()
    {
        if (!_spMap.empty())
            return; 

        // inserts are presorted for rb-tree, names (enum) are presorted too.
        _spMap.insert(make_pair("GATEWAY_INTERFACE", ePARAM_GATEWAY_INTERFACE)); //13

        _spMap.insert(make_pair("HTTP_COOKIE",     ePARAM_HTTP_COOKIE));         //6
        _spMap.insert(make_pair("REQUEST_URI",     ePARAM_REQUEST_URI));         //20

        _spMap.insert(make_pair("DOCUMENT_ROOT",   ePARAM_DOCUMENT_ROOT));       //2
        _spMap.insert(make_pair("HTTP_REFERER",    ePARAM_HTTP_REFERER));        //10
        _spMap.insert(make_pair("REDIRECT_STATUS", ePARAM_REDIRECT_STATUS));     //16
        _spMap.insert(make_pair("SERVER_NAME",     ePARAM_SERVER_NAME));         //24

        _spMap.insert(make_pair("CONTENT_LENGTH",  ePARAM_CONTENT_LENGTH));          //1
        _spMap.insert(make_pair("HTTP_ACCEPT_CHARSET", ePARAM_HTTP_ACCEPT_CHARSET)); //4
        _spMap.insert(make_pair("HTTP_IF_NONE_MATCH", ePARAM_HTTP_IF_NONE_MATCH));   //8
        _spMap.insert(make_pair("HTTP_USER_AGENT", ePARAM_HTTP_USER_AGENT));         //12
        _spMap.insert(make_pair("QUERY_STRING",    ePARAM_QUERY_STRING));            //15
        _spMap.insert(make_pair("REMOTE_ADDR",     ePARAM_REMOTE_ADDR));             //18
        _spMap.insert(make_pair("SCRIPT_NAME",     ePARAM_SCRIPT_NAME));             //22
        _spMap.insert(make_pair("SERVER_PROTOCOL", ePARAM_SERVER_PROTOCOL));         //26

        _spMap.insert(make_pair("CONTENT_TYPE",    ePARAM_CONTENT_TYPE));                 //0
        _spMap.insert(make_pair("HTTP_ACCEPT",     ePARAM_HTTP_ACCEPT));                  //3
        _spMap.insert(make_pair("HTTP_ACCEPT_LANGUAGE", ePARAM_HTTP_ACCEPT_LANGUAGE));    //5
        _spMap.insert(make_pair("HTTP_IF_MODIFIED_SINCE", ePARAM_HTTP_IF_MODIFIED_SINCE));//7
        _spMap.insert(make_pair("HTTP_KEEP_ALIVE", ePARAM_HTTP_KEEP_ALIVE));              //9
        _spMap.insert(make_pair("HTTP_HOST",       ePARAM_HTTP_HOST));                    //11
        _spMap.insert(make_pair("PATH_INFO",       ePARAM_PATH_INFO));                    //14
        _spMap.insert(make_pair("REMOTE_PORT",     ePARAM_REMOTE_PORT));                  //17
        _spMap.insert(make_pair("REQUEST_METHOD",  ePARAM_REQUEST_METHOD));               //19
        _spMap.insert(make_pair("SCRIPT_FILENAME", ePARAM_SCRIPT_FILENAME));              //21
        _spMap.insert(make_pair("SERVER_ADDR",     ePARAM_SERVER_ADDR));                  //23
        _spMap.insert(make_pair("SERVER_PORT",     ePARAM_SERVER_PORT));                  //25
        _spMap.insert(make_pair("SERVER_SOFTWARE", ePARAM_SERVER_SOFTWARE));              //27

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
        if (idx==ePARAM_UNKNOWN) 
            _uParams.insert(make_pair(name, value));            
        else {
            _kParams[idx]=value;
        }
    }

    void Environment::processParams()
    {
        setRequestMethod(getParam(ePARAM_REQUEST_METHOD));
        parseURI(getParam(ePARAM_QUERY_STRING).begin(),
                 getParam(ePARAM_QUERY_STRING).end(),
                 inserter(_getRequest, _getRequest.begin()));
        std::istringstream(getParam(ePARAM_CONTENT_LENGTH)) >> _contentLength;
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
