#ifndef _FCGI_Request_h_
#define _FCGI_Request_h_

#include <string>
#include <sstream>
#include <iostream>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/iostreams/stream.hpp>

#include "Transceiver.hpp"
#include "FcgiSink.hpp"

namespace fcgi
{
    class Transceiver;
    class RequestBase
    {
    public:
        RequestBase(boost::shared_ptr<Transceiver> &tr) :
            _ostream(*this, true),
            _estream(*this, false),
            _tr(tr), _id(0)
        {}

        uint32_t getFd() const { return _fd; }
        uint16_t getId() const { return _id; }
        uint8_t  getFlags() const { return _flags; }
        Role     getRole() const { return _role; }
        bool isKeepConnection() const { return (_flags & FCGI_KEEP_CONN)!=0; }

        std::ostream& outstream() { return _ostream; }
        std::ostream& errstream() { return _estream; }

        void requestComplete(uint32_t status);

    protected:
        typedef boost::iostreams::stream<FcgiSink> Stream;

        friend class FcgiSink;

        void flush(boost::shared_ptr<Block> &blk);
        void flushStreams();

        boost::shared_ptr<Block>  _outStreamBlk;
        boost::shared_ptr<Block>  _errStreamBlk;
        Stream           _ostream;
        Stream           _estream;

        boost::shared_ptr<Transceiver> _tr;

        uint32_t     _fd;
        uint16_t     _id;
        Role         _role;
        uint8_t      _flags;
    };

    template<typename AppHandlerT>
    class RequestHandler : boost::noncopyable, public RequestBase
    {
        typedef RequestHandler<AppHandlerT> ThisType;
    public:
        RequestHandler(boost::shared_ptr<Transceiver> &tr, boost::shared_ptr<Message> &msg) :
            RequestBase(tr),
            _appHandler(*this)
        {
            init(msg);
        }

        void handleParams(boost::shared_ptr<Message> &msg) {
            size_t size=msg->size();
            if (size) {
                const char* data=msg->getData<char>();
                size_t i=0;
                while (i<size) {
                    uint32_t nameLen=0;
                    uint32_t valueLen=0;
                    i+=getNVLength(data+i, nameLen);
                    i+=getNVLength(data+i, valueLen);
                    std::string name(data+i, nameLen);
                    std::string value(data+i+nameLen, valueLen);
                    i+=nameLen+valueLen;
                    _appHandler.params(name, value);
                }
            }
            else
                _appHandler.processParams();
        }

        void handleIN(boost::shared_ptr<Message> &msg) {
            _appHandler.postData(msg->getData<char>(), msg->size());
            if (msg->size()==0)
                _appHandler.processRequest();
        }
        
        void handleData(boost::shared_ptr<Message> &msg) {
            _appHandler.data(msg->getData<char>(), msg->size());
        }

        void handleAbort(boost::shared_ptr<Message> &msg) {
            _appHandler.abort();
        }

        void handleError(const Exceptions::FcgiException& ex) {
            _appHandler.handleError(ex);
        }

    private:
        void init(boost::shared_ptr<Message> &msg)
        {
            _id = msg->getId();
            const BeginRequestRecord* ptr=msg->getData<BeginRequestRecord>();
            _role = static_cast<Role>(ptr->role());
            _flags = ptr->_flags;
        }

        boost::shared_ptr<Transceiver> _tr;
        AppHandlerT _appHandler;
    };

}

#endif
