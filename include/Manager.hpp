#ifndef _Fcgi_Manager_h_
#define _Fcgi_Manager_h_

#include <map>
#include <boost/unordered_map.hpp>
#include <iterator>

#include "Transceiver.hpp"
#include "Request.hpp"

#define FCGI_DEBUG

namespace fcgi
{
    template<typename ManagerT>
    class Acceptor
    {
        typedef Acceptor<ManagerT> ThisType;
    public:
        Acceptor(ManagerT& manager, uint32_t fd, struct ev_loop* loop=EV_DEFAULT)
            : _manager(manager), _fd(fd), _rev(loop)
        {
            _rev.set < ThisType, &ThisType::evRead  > (this);
            _rev.start(_fd, ev::READ);
        }

    private:
        ManagerT& _manager;
        uint32_t  _fd;
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
                        throw Exceptions::SocketAccept(_fd, erno);
                    }
                    else {
                        // notify that we have a new connection
                        _manager.accept(fd);
                    }
                }
            }
            catch (const Exceptions::Socket& ex) {
                _manager.acceptError(ex);
            }
        }
    };

    class ManagerHandle
    {
    public:
        virtual void handle(boost::shared_ptr<Transceiver> &tr,
                            boost::shared_ptr<Message> &message)=0;

        virtual void requestComplete(uint32_t status, uint32_t fd,
                                     uint16_t id, bool keepConnection)=0;

        virtual void acceptError(const Exceptions::Socket& ex) =0;
        virtual void readError(const Exceptions::Socket& ex) =0;
        virtual void writeError(const Exceptions::Socket& ex) =0;
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
        typedef typename boost::unordered_map< uint16_t,
                                               boost::shared_ptr<HandlerType> > RequestsMap;
        typedef typename std::map< uint32_t, RequestsMap > RequestsType;
        
        Manager(uint32_t fd=0, struct ev_loop* loop=EV_DEFAULT) :
            _maxConns(100), _maxReqs(100), _mpxsConns(10), _loop(loop),
            _acceptor(*this, fd, loop)
        {}

        virtual void handle(boost::shared_ptr<Transceiver> &tr,
                            boost::shared_ptr<Message> &message)
        {
            RecordType type = static_cast<RecordType>(message->type());
            uint32_t fd = message->getFd();
            uint16_t id = message->getId();

            typename RequestsType::iterator cIt=_reqs.find(fd), cEnd=_reqs.end();
            typename RequestsMap::iterator  rIt,rEnd;
            bool isReqs=false;

            // cIt - connection (fd) iterator
            // rIt - request (req id) iterator
            if (cIt!=cEnd) {
                rIt=(*cIt).second.find(id);
                rEnd=(*cIt).second.end();
                isReqs=true;
            }
#ifdef FCGI_DEBUG
            std::cout<<"FCGI ["<<fd<<"] handle message type="<<type<<", id="<< id
                     <<((isReqs && rIt!=rEnd)  ? " FOUND" : " NOT FOUND")<<std::endl;
#endif
            switch (type) {
            case BEGIN_REQUEST :
                if (isReqs && rIt==rEnd) {
                    boost::shared_ptr<HandlerType> rq(new HandlerType(tr,message));
                    cIt->second.insert(make_pair(id, rq));
                }
                else 
                    throw Exceptions::FcgiException("BEGIN_REQUEST on the same connection with id already exists.",0);
                break;
            case PARAMS:
                if (isReqs && rIt!=rEnd) {
                    rIt->second->handleParams(message);
                }
                break;
            case IN:
                if (isReqs && rIt!=rEnd) {
                    rIt->second->handleIN(message);
                }
                break;
            case DATA:
                if (isReqs && rIt!=rEnd) {
                    rIt->second->handleData(message);
                }
                break;
            case ABORT_REQUEST:
                if (isReqs && rIt!=rEnd) {
                    rIt->second->handleAbort(message);
                    cIt->second.erase(rIt);
                }
                break;
            case GET_VALUES:
                processGetValues(tr, message);
                break;
            default:
                break;
            }
        }

        virtual void acceptError(const Exceptions::Socket& ex)
        {
            // TODO: socket::accept error
            exit(-1);
        }

        void errorNotify(const Exceptions::Socket& ex)
        {
            typename RequestsType::iterator mIt=_reqs.find(ex._fd), mEnd=_reqs.end();
            if (mIt!=mEnd) {
                typename RequestsMap::iterator rIt=mIt->second.begin(), rEnd=mIt->second.end();
                for (;rIt!=rEnd;++rIt) 
                    rIt->second->handleError(ex);
            }
        }

        virtual void readError(const Exceptions::Socket& ex)
        {
            errorNotify(ex);
            close(ex._fd);
        }

        virtual void writeError(const Exceptions::Socket& ex)
        {
            errorNotify(ex);
            close(ex._fd);
        }

        void processGetValues(boost::shared_ptr<Transceiver> &tr,
                              boost::shared_ptr<Message> &message)
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
                        if (fcgiMaxConns.length()==nameLen &&
                            !memcmp(data+i, fcgiMaxConns.c_str(), nameLen)) {
                            std::ostringstream reply; reply << _maxConns;
                            buffer+=(char)nameLen + (char)(reply.str().size());
                            buffer+=fcgiMaxConns;
                            buffer+=reply.str();
                        }
                        if (fcgiMaxReqs.length()==nameLen &&
                            !memcmp(data+i, fcgiMaxReqs.c_str(), nameLen)) {
                            std::ostringstream reply; reply << _maxReqs;
                            buffer+=(char)nameLen + (char)(reply.str().size());
                            buffer+=fcgiMaxReqs;
                            buffer+=reply.str();
                        }
                        if (fcgiMpxsConns.length()==nameLen &&
                            !memcmp(data+i, fcgiMpxsConns.c_str(), nameLen)) {
                            std::ostringstream reply; reply << _mpxsConns;
                            buffer+=(char)nameLen + (char)(reply.str().size());
                            buffer+=fcgiMpxsConns;
                            buffer+=reply.str();
                        }
                    }
                    else {
                        throw Exceptions::FcgiException("Invalid GET_VALUE request", 0);
                    }
                    i+=nameLen;
                }
                boost::shared_ptr<Block> reply(Block::valuesReply(message->getId(), buffer));
                reply->owner()=tr;
                tr->writeBlock(reply);
            }
        }

        virtual void requestComplete(uint32_t status, uint32_t fd, uint16_t id, bool keepConnection)
        {
            typename RequestsType::iterator mIt=_reqs.find(fd), mEnd=_reqs.end();
            if (mIt!=mEnd) {
                mIt->second.erase(id);
            }
            if (!keepConnection) {
                close(fd);
            }
        }

        void remove(uint32_t fd, uint16_t id)
        {
            typename RequestsType::iterator mIt=_reqs.find(fd), mEnd=_reqs.end();
            typename RequestsMap::iterator  rIt,rEnd;
            if (mIt!=mEnd)
                rIt=(*mIt).second.erase(id);
        }

        void close(uint32_t fd) {
            _reqs.erase(fd);
            ConnsType::iterator cIt=_conns.find(fd);
            if (cIt!=_conns.end()) {
                // cIt->second->close();
                _conns.erase(cIt);
            }
        }

        void accept(uint32_t fd) {
            boost::shared_ptr<Transceiver> conn(new Transceiver(*this, fd, _loop));
            _conns.insert(std::make_pair(fd, conn));
            _reqs.insert(std::make_pair(fd, RequestsMap()));
#ifdef FCGI_DEBUG
            std::cout<<"FCGI ["<<fd<<"] accept, total connections:"<<_conns.size()<<std::endl;
#endif
        }

        uint32_t     _maxConns;
        uint32_t     _maxReqs;
        uint32_t     _mpxsConns;

        struct ev_loop* _loop;
        
        AcceptorType _acceptor;

        // all connection
        ConnsType    _conns;

        // all requests
        RequestsType _reqs;
    };
}

#endif
