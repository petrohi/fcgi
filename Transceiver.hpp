#ifndef _FCGI_Transceiver_h_
#define _FCGI_Transceiver_h_

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/un.h>

#include <exception>
#include <queue>

#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <ev++.h>

#include "Protocol.hpp"
#include "Exceptions.hpp"
#include "Block.hpp"

namespace fcgi
{
    class Buffer : boost::noncopyable
    {
    public:
        Buffer() : _expect(0) {}

        char* ptr() const {
            return static_cast<char*>(_data.get())+(_header->recordLength()-_expect);
        }
        
        size_t expect() const { return _expect; }
        bool is_complete() const { return (_expect==0); }
        void update(size_t aread) { _expect-=aread; }

        void clear() { _header.reset(); _expect=0; _data.reset(); }
        void setHeader(boost::shared_ptr<RecordHeader> &header) {
            _header=header;
            _expect=_header->recordLength();
            _data.reset(new char[_expect]);
        }

        friend struct Message;

    private:
        size_t _expect;
        boost::shared_ptr<RecordHeader> _header;
        boost::shared_array<char> _data;
    };

    class Message : boost::noncopyable
    {
    public:
        Message(uint32_t fd, Buffer& buffer) 
            : _fd(fd),
              _requestId(buffer._header->requestId()),
              _size(buffer._header->contentLength()),
              _type(buffer._header->type()),
              _data(buffer._data)
        {}

        uint32_t getFd() const { return _fd; }
        uint32_t getId() const { return constructFullId(_fd, _requestId); }
        uint16_t requestId() const { return _requestId; }
        
        uint16_t type() const { return _type; }
        uint32_t size() const { return static_cast<uint32_t>(_size); }

        template<typename Type>
        const Type* getData() const {
            return (const Type*)(_data.get());
        }

    private:
        uint32_t   _fd;
        uint16_t   _requestId;
        uint16_t   _size;
        RecordType _type;
        boost::shared_array<char> _data;
    };


    class ManagerHandle;
    class Transceiver : public boost::enable_shared_from_this<Transceiver>
    {
        typedef Transceiver ThisType;
    public:
        Transceiver(ManagerHandle& manager, int fd);
        ~Transceiver();

        uint32_t getFd() const { return _fd; }

        void requestComplete(uint32_t status, uint32_t fullId, bool keepConnection);

        void close();

        void writeStream(uint16_t id, std::string& str, bool isOut=true);
        void writeBlock(boost::shared_ptr<Block> &blk);

    private:
        static 
        void nonblock(int fd)
        {
            fcntl(fd, F_SETFL,(fcntl(fd, F_GETFL)|O_NONBLOCK)|O_NONBLOCK);
        }

        void resetReadHeader();

        class EvIOLock : boost::noncopyable
        {
        public:
            EvIOLock(ev::io& watcher) :
                _watcher(watcher)
            {
                _watcher.stop();
            }
            ~EvIOLock() {
                _watcher.start();
            }
        private:
            ev::io& _watcher;
        };

        void evRead(ev::io &watcher, int revents);
        bool readHeader();
        bool readRecord();
        size_t readdata(void* ptr, size_t expect);
        void writeHead();

        void evWrite(ev::io &watcher, int wevents);

        ManagerHandle&  _manager;
        int             _fd;
        struct ev_loop* _loop;
        ev::io          _rev;
        ev::io          _wev;
        bool            _acceptor;
        bool            _is_header;
        bool            _writeHeader;
        size_t          _expectRead;
        size_t          _expectWrite;

        boost::shared_ptr<RecordHeader> _header;
        Buffer          _buffer;
        std::queue<boost::shared_ptr<Block> > _queue;
    };

}
#endif
