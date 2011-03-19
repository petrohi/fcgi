#ifndef _Fcgi_Manager_h_
#define _Fcgi_Manager_h_

#include <map>
#include <boost/unordered_map.hpp>
#include <iterator>

#include "Transceiver.hpp"
#include "Request.hpp"

namespace fcgi
{
    template<typename ManagerT>
    class Acceptor
    {
        typedef Acceptor<ManagerT> ThisType;
    public:
        Acceptor(ManagerT& manager, int fd)
            : _manager(manager), _fd(fd)
        {
            _rev.set < ThisType, &ThisType::evRead  > (this);
            _rev.start(_fd, ev::READ);
        }

    private:
        ManagerT& _manager;
        int       _fd;
        ev::io    _rev;

        void evRead(ev::io &watcher, int revents)
        {
            try {
                while (true) {
                    union {
                        sockaddr    sa;
                        sockaddr_un un;
                        sockaddr_in in;
                    }  addr;
                    socklen_t  addrlen=sizeof(addr);
                    int fd = accept4(_fd, &addr.sa, &addrlen, SOCK_NONBLOCK);
                    int erno=errno;
                    if (fd==-1) {
                        if (erno==EAGAIN || erno==EWOULDBLOCK)
                            return;
                        throw Exceptions::SocketAccept(_fd,erno);
                    }
                    else {
                        // notify that we have a new connection
                        _manager.accept(fd);
                    }
                }
            }
            catch (const Exceptions::Socket& ex) {
            }
        }
    };

    class ManagerHandle
    {
    public:
        virtual void handle(boost::shared_ptr<Transceiver> &tr,
                            boost::shared_ptr<Message> &message)=0;

        virtual void requestComplete(uint32_t status, uint32_t fullId, bool keepConnection)=0;
    };
    
    template<typename AppHandlerT>
    class Manager : public ManagerHandle, boost::noncopyable
    {
    public:
        typedef AppHandlerT                                      AppHandlerType;
        typedef Manager<AppHandlerType>                          ManagerType;
        typedef Acceptor<ManagerType>                            AcceptorType;
        typedef RequestHandler<AppHandlerType>                   HandlerType;
        
        typedef typename std::map< uint32_t, boost::shared_ptr<Transceiver> >   ConnsType;
        typedef typename boost::unordered_map< uint32_t,
                                               boost::shared_ptr<HandlerType> > RequestsType;
        
        Manager() :
			_maxConns(100), _maxReqs(100), _mpxsConns(10),
			_acceptor(*this, 0)
		{}
        
        virtual void handle(boost::shared_ptr<Transceiver> &tr,
                            boost::shared_ptr<Message> &message)
        {
            RecordType type = static_cast<RecordType>(message->type());
            uint32_t reqId = message->getId();
            typename RequestsType::iterator rIt(_reqs.find(reqId));

            std::cout<<"Message type "<<type<<" id "<< (0xffff & reqId)
                     <<(rIt==_reqs.end() ? " NOT FOUND" : " FOUND")<<std::endl;

            switch (type) {
            case BEGIN_REQUEST :
                if (rIt==_reqs.end()) {
                    boost::shared_ptr<HandlerType> rq(new HandlerType(tr,message));
                    _reqs.insert(make_pair(reqId, rq));
                }
                else {
                    // already exists!!!
                }
                break;
            case PARAMS:
                if (rIt!=_reqs.end()) {
                    rIt->second->handleParams(message);
                }
                break;
            case IN:
                if (rIt!=_reqs.end()) {
                    rIt->second->handleIN(message);
                }
                break;
            case DATA:
                if (rIt!=_reqs.end()) {
                    rIt->second->handleData(message);
                }
                break;
            case ABORT_REQUEST:
                if (rIt!=_reqs.end()) {
                    rIt->second->handleAbort(message);
                    _reqs.erase(rIt);
                }
                break;
            case GET_VALUES:
                break;
            default:
                break;
            }
        }
        void processGetValues(boost::shared_ptr<Transceiver> &tr, boost::shared_ptr<Message> &message)
        {
            size_t size=message->size();
            if (size) {
                std::string buffer; buffer.reserve(size*2);
                const char* data=message->getData<char>();
                size_t i=0;
                // vector<const string&> _names(3);
                while (i<size) {
                    uint32_t nameLen=0;
                    uint32_t valueLen=0;
                    i+=getNVLength(data+i, nameLen);
                    i+=getNVLength(data+i, valueLen);
                    if (valueLen==0 && nameLen>0 && i+nameLen <= size) {
                        if (fcgiMaxConns.length()==nameLen && !memcmp(data+i, fcgiMaxConns.c_str(), nameLen)) {
                            std::ostringstream reply; reply << _maxConns;
                            buffer+=(char)nameLen + (char)(reply.str().size());
                            buffer+=fcgiMaxConns;
                            buffer+=reply.str();
                        }
                        if (fcgiMaxReqs.length()==nameLen && !memcmp(data+i, fcgiMaxReqs.c_str(), nameLen)) {
                            std::ostringstream reply; reply << _maxReqs;
                            buffer+=(char)nameLen + (char)(reply.str().size());
                            buffer+=fcgiMaxReqs;
                            buffer+=reply.str();
                        }
                        if (fcgiMpxsConns.length()==nameLen && !memcmp(data+i, fcgiMpxsConns.c_str(), nameLen)) {
                            std::ostringstream reply; reply << _mpxsConns;
                            buffer+=(char)nameLen + (char)(reply.str().size());
                            buffer+=fcgiMpxsConns;
                            buffer+=reply.str();
                        }
                    }
                    else
                        throw std::exception("Invalid GET_VALUE request");
                    i+=nameLen;
                }
                boost::shared_ptr<Block> reply(Block::valuesReply(message->getId(), buffer));
                reply->owner()=tr;
                tr->writeBlock(reply);
            }
        }

        virtual void requestComplete(uint32_t status, uint32_t fullId, bool keepConnection)
        {
            uint32_t fd=((fullId>>16)&0xffff);
            _reqs.erase(fullId);
            if (!keepConnection)
                close(fd);
        }

        void close(uint32_t fd) {
            ConnsType::iterator cIt=_conns.find(fd);
            if (cIt!=_conns.end()) {
                // cIt->second->close();
                _conns.erase(cIt);
            }
        }
    
        void accept(uint32_t fd) {
            boost::shared_ptr<Transceiver> conn(new Transceiver(*this,fd));
            _conns.insert(std::make_pair(fd, conn));
        }

        uint32_t     _maxConns;
        uint32_t     _maxReqs;
        uint32_t     _mpxsConns;
        
        AcceptorType _acceptor;

        // all connection
        ConnsType    _conns;

        // all requests
        RequestsType _reqs;
    };
}

#endif
