#include <iostream>
#include "Transceiver.hpp"
#include "Manager.hpp"
#include "Protocol.hpp"

namespace fcgi {

    Transceiver::Transceiver(ManagerHandle& manager, int fd) :
        _manager(manager),
        _fd(fd),
        _loop(EV_DEFAULT),
        _is_header(true),
        _expectRead(sizeof(RecordHeader)),
        _header(new RecordHeader())
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

    void Transceiver::resetReadHeader()
    {
        _header.reset(new RecordHeader());
        _expectRead=sizeof(RecordHeader);
        _is_header=true;
    }

    void Transceiver::evRead(ev::io &watcher, int revents)
    {
        // keep it to make sure to exists till the end.
        boost::shared_ptr<ThisType> This(this->shared_from_this());

        std::cout << "evRead "<<_fd<<std::endl;
        EvIOLock lock(watcher);

        try {
            if (_is_header) {
                if (!readHeader())
                    return;
                // init the buffer
                _buffer.setHeader(_header);
                std::cout << "header "; _header->print(std::cout);
            }
            if (readRecord()) {
                {
                    boost::shared_ptr<Message> msg(new Message(_fd, _buffer));
                    _manager.handle(This, msg);
                }
                // release buffer
                // _buffer.clear();
                resetReadHeader();
            }
        }
        catch (const Exceptions::Socket& ex) {
            std::cout << "FD "<<_fd<<" Exception " << ex.what() << std::endl;
            exit(-1);
        }
    }

    bool Transceiver::readHeader()
    {
        size_t aread = readdata(_header.get()+(sizeof(RecordHeader)-_expectRead), _expectRead);
        if (aread==_expectRead) {
            _is_header=false;
            return true;
        }
        else {
            _expectRead-=aread;
            return false;
        }
    }

    bool Transceiver::readRecord()
    {
        std::cout << "read RecordBody size " << _buffer.expect() << std::endl;
        if (_buffer.expect()>0) {
            size_t aread = readdata(_buffer.ptr(), _buffer.expect());
            _buffer.update(aread);
        }
        return _buffer.is_complete();
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
                size_t offset;
                const char* ptr;
                if (_writeHeader) {
                    offset=blk->getHeader().size()-_expectWrite;
                    ptr=blk->getHeader().data()+offset;
                }
                else {
                    offset=blk->getData().size()-_expectWrite;
                    ptr=blk->getData().data()+offset;
                }

                std::cout<<"DATA"<<std::endl;
                for (int i=0; i<_expectWrite; ++i)
                    std::cout<<" "<<(int)(*((uint8_t*)ptr+i));
                std::cout<<std::endl;

                int result = write(_fd, ptr, _expectWrite);
                int erno = errno;
                std::cout << "evwrite "<<_fd<<" size "<<_expectWrite
                          <<" written "<<result<<std::endl;

                if (result!=-1) {
                    _expectWrite-=result;
                    if (_expectWrite==0) {
                        if (_writeHeader) {
                            _writeHeader=false;
                            _expectWrite=blk->getData().size();
                        }
                        else {
                            _queue.pop();
                            writeHead();
                        }
                    }
                }
                else {
                    if (erno == EAGAIN)
                        return;
                    throw Exceptions::SocketWrite(_fd, erno);
                }
            }
            _wev.stop();
        }
        catch (const Exceptions::SocketWrite& ex) {
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

    void Transceiver::writeStream(uint16_t id, std::string& str, bool isOut)
    {
        size_t left = str.size();
        if (left <= (65536 - sizeof(RecordHeader))) {
            boost::shared_ptr<Block> data(Block::stream(id, str, isOut));
            data->owner()=shared_from_this();
            writeBlock(data);
        }
    }

}
