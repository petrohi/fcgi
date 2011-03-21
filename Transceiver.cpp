#include <iostream>
#include "Transceiver.hpp"
#include "Manager.hpp"
#include "Protocol.hpp"

namespace fcgi {

    Transceiver::Transceiver(ManagerHandle& manager, int fd) :
        _manager(manager),
        _fd(fd),
        _loop(EV_DEFAULT)
    {
        nonblock(_fd);
        _rev.set < ThisType, &ThisType::evRead  > (this);
        _wev.set < ThisType, &ThisType::evWrite > (this);
        _rev.start(_fd, ev::READ);
        _wev.set(_fd, ev::WRITE); // do not start
        std::cout << "Transceiver::new transceiver " << _fd << std::endl;
    }

    void Transceiver::close() {
        _rev.stop();
        _wev.stop();
        if (_fd!=-1) {
            ::close(_fd);
            _fd=-1;
        }
    }
    Transceiver::~Transceiver() {
        close();
    }

    void Transceiver::evRead(ev::io &watcher, int revents)
    {
        // stop reader notifications
        EvIOLock lock(watcher);

        // keep it to make sure to exists till the end.
        boost::shared_ptr<ThisType> This(this->shared_from_this());

        std::cout << "evRead "<<_fd<<std::endl;

        try {
            while (!_readBuffer.is_complete()) {
                size_t aread = readdata(_readBuffer.ptr(), _readBuffer.expect());
                if (aread==0)
                    return;
                _readBuffer.update(aread);
            }
            {
                boost::shared_ptr<Message> msg(new Message(_fd, _readBuffer));
                _manager.handle(This, msg);
            }
            // release buffer
            _readBuffer.reset();
        }
        catch (const Exceptions::Socket& ex) {
            std::cout << "evRead FD="<<_fd<<" Exception " << ex.what() << std::endl;
            // TODO: pass an error to the Manager
        }
    }

    size_t Transceiver::readdata(void* ptr, size_t expect)
    {
        int result=read(_fd, ptr, expect);
        int erno=errno;

        if (result>0) {
            std::cout << "read got " << result << " expected "<< expect << std::endl;
            if (result<=expect) {
                return result;
            }
            assert(result<=expect); // read more than asked.
        }
        else {
            std::cout << "read got error " << erno << std::endl;
            if (erno==EAGAIN || erno==EWOULDBLOCK)
                return 0;
            throw Exceptions::SocketRead(_fd, erno);
        }
    }

    void Transceiver::writeHead() {
        if (!_queue.empty()) {
            _writeHeader=!(_queue.front()->isOnePiece());
            _expectWrite=(_writeHeader ?
                          _queue.front()->getHeader().size() : _queue.front()->getData().size());
        }
    }

    void Transceiver::writeBlock(boost::shared_ptr<Block> &blk) {
        bool was_empty = _queue.empty();
        _queue.push(blk);
        if (was_empty) {
            writeHead();
            _wev.start();
            std::cout << "writeBlock"<<std::endl;
        }
    }

    void Transceiver::evWrite(ev::io &watcher, int wevents) {
        boost::shared_ptr<Block> blk;
        std::cout << "evwrite "<<_fd<<std::endl;
        try {
            if (_queue.empty()) {
                _wev.stop();
                return;
            }
            while(!_queue.empty()) {
                blk=_queue.front();
                while (!_expectWrite==0) {
                    const char* ptr;
                    if (_writeHeader)
                        ptr=blk->getHeader().data()+(blk->getHeader().size()-_expectWrite);
                    else
                        ptr=blk->getData().data()+(blk->getData().size()-_expectWrite);

                    size_t awrite=write(_fd, ptr, _expectWrite);
                    int erno = errno;
                    std::cout << "evwrite "<<_fd<<" size "<<_expectWrite
                              <<" written "<<awrite<<std::endl;

                    if (awrite!=-1) {
                        _expectWrite-=awrite;
                        if (_expectWrite==0 && _writeHeader) {
                            _expectWrite=blk->getData().size();
                            _writeHeader=false;
                        }
                    }
                    else {
                        if (erno == EAGAIN)
                            return;
                        throw Exceptions::SocketWrite(_fd, erno);
                    }
                }
                _queue.pop();
                writeHead();
            }
            _wev.stop();
        }
        catch (const Exceptions::SocketWrite& ex) {
            std::cout << "evWrite, FD="<<_fd<<" Exception " << ex.what() << std::endl;
            // TODO: pass an error to the Manager
        }
    }

    void Transceiver::requestComplete(uint32_t status, uint32_t fullId, bool keepConnection)
    {
        boost::shared_ptr<Block>
            reply(Block::endRequest(status, static_cast<uint16_t>(0xffff&fullId), REQUEST_COMPLETE));
        reply->owner()=shared_from_this();
        writeBlock(reply);
        _manager.requestComplete(status, fullId, keepConnection);
    }

#if 0
    void Transceiver::writeStream(uint16_t id, std::string& str, bool isOut)
    {
        size_t left = str.size();
        if (left <= (65536 - sizeof(RecordHeader))) {
            boost::shared_ptr<Block> data(Block::stream(id, str, isOut));
            data->owner()=shared_from_this();
            writeBlock(data);
        }
    }
#endif

}
