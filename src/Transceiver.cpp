#include <iostream>
#include "Transceiver.hpp"
#include "Manager.hpp"
#include "Protocol.hpp"

namespace fcgi {

    Transceiver::Transceiver(ManagerHandle& manager, int fd, struct ev_loop* loop) :
        _manager(manager),
        _fd(fd),
        // _loop(loop),
        _rev(loop),
        _wev(loop)
    {
        nonblock(_fd);
        _rev.set < ThisType, &ThisType::evRead  > (this);
        _wev.set < ThisType, &ThisType::evWrite > (this);
        _rev.start(_fd, ev::READ);
        _wev.set(_fd, ev::WRITE); // do not start
        // std::cout << "Transceiver::new transceiver " << _fd << std::endl;
    }

    void Transceiver::close()
    {
        if (_fd!=-1) {
            _rev.stop();
            _wev.stop();

            std::queue<boost::shared_ptr<Block> > empty;
            std::swap(empty, _queue);
        
            ::close(_fd);
            _fd=-1;
        }
    }

    Transceiver::~Transceiver()
    {
        close();
    }

    void Transceiver::evRead(ev::io &watcher, int revents)
    {
        // stop reader notifications
        EvIOLock lock(watcher);

        // keep it to make sure to exists till the end.
        boost::shared_ptr<ThisType> This(this->shared_from_this());

        // std::cout << "evRead "<<_fd<<std::endl;

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
            _manager.readError(ex);
            // close(); // do not close here, Manager call close() 
        }
    }

    size_t Transceiver::readdata(void* ptr, size_t expect)
    {
        int result=recv(_fd, ptr, expect, MSG_NOSIGNAL);
        int erno=errno;

        if (result>0) {
            // std::cout << "read got " << result << " expected "<< expect << std::endl;
            if (result<=expect) {
                return result;
            }
            assert(result<=expect); // read more than asked.
        }
        else {
            // std::cout << "read got error " << erno << std::endl;
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
            // std::cout << "writeBlock"<<std::endl;
        }
    }

    void Transceiver::evWrite(ev::io &watcher, int wevents) {
        boost::shared_ptr<Block> blk;
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

                    size_t awrite=send(_fd, ptr, _expectWrite, MSG_NOSIGNAL);
                    int erno = errno;
                    // std::cout << "evwrite "<<_fd<<" size "<<_expectWrite
                    //          <<" written "<<awrite<<std::endl;

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
            _manager.writeError(ex);
            // close(); // do not close here, Manager call close() 
        }
    }

    void Transceiver::requestComplete(uint32_t status, uint16_t id, bool keepConnection)
    {
        boost::shared_ptr<Block>
            reply(Block::endRequest(status, id, REQUEST_COMPLETE));
        reply->owner()=shared_from_this();
        writeBlock(reply);
        _manager.requestComplete(status, _fd, id, keepConnection);
    }

}
