#ifndef _FCGI_Protocol_h_
#define _FCGI_Protocol_h_

#include <iostream>
#include <boost/noncopyable.hpp>
#include <netinet/in.h>

#define FCGI_MAX_CONNS  "FCGI_MAX_CONNS"
#define FCGI_MAX_REQS   "FCGI_MAX_REQS"
#define FCGI_MPXS_CONNS "FCGI_MPXS_CONNS"

namespace fcgi
{
    const uint8_t fcgiProtoVersion=1;

    extern const std::string fcgiMaxConns;
    extern const std::string fcgiMaxReqs;
    extern const std::string fcgiMpxsConns;

    typedef enum _RecordType {
        BEGIN_REQUEST=1,
        ABORT_REQUEST=2,
        END_REQUEST=3,
        PARAMS=4,
        IN=5,
        OUT=6,
        ERR=7,
        DATA=8,
        GET_VALUES=9,
        GET_VALUES_RESULT=10,
        UNKNOWN_TYPE=11
    } RecordType;

    typedef enum _Role {
        RESPONDER=1,
        AUTHORIZER=2,
        FILTER=3
    } Role;

    typedef enum _Status {
        REQUEST_COMPLETE=0,
        CANT_MPX_CONN=1,
        OVERLOADED=2,
        UNKNOWN_ROLE=3
    } Status;

    typedef enum _RequestFlags {
        FCGI_KEEP_CONN=1
    } RequestFlags;

    inline
    uint32_t constructFullId(uint32_t fd, uint16_t recId) { return ((0xffff & fd) << 16) | recId; }

    size_t getNVLength(const char* data, uint32_t& len);

    class RecordHeader : boost::noncopyable
    {
    public:
        RecordHeader()  :
            _version(0), _type(0),
            _requestIdB1(0),
            _requestIdB0(0),
            _contentLengthB1(0),
            _contentLengthB0(0),
            _paddingLength(0),
            _reserved(0) 
        {}
        RecordHeader(uint8_t version, RecordType type, uint16_t id, uint16_t length) :
            _version(version), _type(static_cast<uint8_t>(type)),
            _paddingLength(0),
            _reserved(0) 
        {
            requestId(id);
            contentLength(length);
        }

        void init(uint8_t ver, RecordType t, uint16_t id, uint16_t length, uint8_t padding=0)
        {
            version(ver);
            type(t);
            requestId(id);
            contentLength(length);
            paddingLength(padding);
            _reserved=0;
        }


        uint8_t version() const {
            return _version;
        }
        RecordType type() const {
            return static_cast<RecordType>(_type);
        }
        uint16_t requestId() const {
            return ntohs(*((uint16_t*)(&_requestIdB1)));
        }
        uint16_t contentLength() const {
            return ntohs(*((uint16_t*)(&_contentLengthB1)));
        }
        uint8_t paddingLength() const {
            return _paddingLength;
        }
        // length of a record without a header (content+padding)
        size_t recordLength() const {
            size_t result=contentLength();
            result+=_paddingLength;
            return result;
        }
        void version(uint8_t ver) { _version=ver; }
        void type(RecordType rType) { _type=static_cast<uint8_t>(rType); }
        void requestId(uint16_t id) {
            *((uint16_t*)(&_requestIdB1))=htons(id);
        }
        void contentLength(uint16_t len) {
            *((uint16_t*)(&_contentLengthB1))=htons(len);
        }
        void paddingLength(uint8_t len) {
            _paddingLength=len;
        }

        std::ostream& print(std::ostream& out) const {
            out<<(int)_version<<","
               <<(int)_type<<","
               <<(int)_requestIdB1<<","
               <<(int)_requestIdB0<<","
               <<(int)_contentLengthB1<<","
               <<(int)_contentLengthB0<<","
               <<(int)_paddingLength<<","
               <<(int)_reserved<<" id "<<requestId()<<" size "<<contentLength()
               <<" total "<<recordLength()
               <<std::endl;
            return out;
        }
    private:
        uint8_t _version;
        uint8_t _type;
        uint8_t _requestIdB1;
        uint8_t _requestIdB0;
        uint8_t _contentLengthB1;
        uint8_t _contentLengthB0;
        uint8_t _paddingLength;
        uint8_t _reserved;
    };
    struct BeginRequestRecord
    {
        uint8_t _roleB1;
        uint8_t _roleB0;
        uint8_t _flags;
        uint8_t _reserved[5];
        Role role() const {
            return static_cast<Role>(ntohs(*(uint16_t*)(&(_roleB1))));
        } 
    };
    struct EndRequestRecord
    {
        uint8_t _appStatusB3;
        uint8_t _appStatusB2;
        uint8_t _appStatusB1;
        uint8_t _appStatusB0;
        uint8_t _protocolStatus;
        uint8_t _reserved[3];
        void status(uint32_t status) {  *((uint32_t*)(&_appStatusB3))=htonl(status); }
        void protoStatus(uint8_t status) { _protocolStatus=status; }
    };
    
}

#endif
