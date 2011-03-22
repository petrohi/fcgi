#ifndef _FCGI_Block_h_
#define _FCGI_Block_h_

#include <boost/noncopyable.hpp>
#include "Protocol.hpp"

namespace fcgi
{
    class Transceiver;
    class Block : boost::noncopyable
    {
    private:
        Block(RecordType type, uint16_t id, std::string& str) :
            _header(sizeof(RecordHeader), '\0')
        {
            getRecordHeader()->init(fcgiProtoVersion, type, id, str.size());
            _data.swap(str);
        }

        Block(RecordType type, uint16_t id, uint16_t len) 
        {
            _data.reserve(len+sizeof(RecordHeader));
            _data.resize(sizeof(RecordHeader), '\0');
            getRecordHeader()->init(fcgiProtoVersion, type, id, 0);
            // std::cout<<"Block("
            //         <<type<<","<<id<<","<<len
            //         <<") size:"<<_data.size()
            //         <<" capacity:"<<_data.capacity()<<std::endl;
        }

        EndRequestRecord* getEndRequestRecord() const {
            return (EndRequestRecord*)(_data.c_str() + sizeof(RecordHeader));
        }

        static Block* createBlock(RecordType type, uint16_t id, std::string& str) {
            // 2 part record
            return new Block(type, id, str);
        }

    public:
        static Block* stream(uint16_t id, bool isOut, uint16_t size) {
            return new Block((isOut ? OUT : ERR), id, size);
        }

        static Block* valuesReply(uint16_t id, std::string& str) {
            return createBlock(GET_VALUES_RESULT, id, str);
        }

        static Block* endRequest(uint32_t status, uint16_t id, uint8_t proto)
        {
            Block* block=new Block(END_REQUEST, id, sizeof(EndRequestRecord));
            block->_data.resize(sizeof(EndRequestRecord)+sizeof(RecordHeader));
            block->getRecordHeader()->contentLength(sizeof(EndRequestRecord));
            EndRequestRecord* request=block->getEndRequestRecord();
            request->status(status);
            request->protoStatus(proto);
            return block;
        }

        bool isOnePiece() const { return _header.empty(); }

        const std::string& getData() const { return _data; }
        const std::string& getHeader() const { return _header; }

        std::string& data() { return _data; }

        char* getDataPtr() {
            return (char*)(_data.data())+
                ((_header.size()==0) ? sizeof(RecordHeader) : 0);
        }

        RecordHeader* getRecordHeader() const {
            if (_header.size()==0)
                return (RecordHeader*)(_data.data());
            else
                return (RecordHeader*)(_header.data());
        }

        boost::shared_ptr<Transceiver>& owner() { return _tr; }

    private:
        boost::shared_ptr<Transceiver> _tr;
        std::string                _header;
        std::string                  _data;
    };
}


#if 0
class Transceiver;
class Block : boost::noncopyable
{
private:
    Block(RecordType type, uint16_t id, uint16_t len) :
        _header(fcgiProtoVersion, type, id, len),
        _data(len, 0)
    {}


    Block(RecordType type, uint16_t id, std::string& str) :
        _header(fcgiProtoVersion, type, id, str.size())
    {
        _data.swap(str);
    }

    static inline
    Block* createBlock(RecordType type, uint16_t id, std::string& str) {
        return new Block(type, id, str);
    }

public:
    static Block* stream(uint16_t id, std::string& str, bool isOut) {
        return new Block((isOut ? OUT : ERR), id, str);
    }

    static Block* outStream(uint16_t id, std::string& str) {
        return createBlock(OUT, id, str);
    }

    static Block* errStream(uint16_t id, std::string& str) {
        return createBlock(ERR, id, str);
    }

    static Block* valuesReply(uint16_t id, std::string& str) {
        return createBlock(GET_VALUES_RESULT, id, str);
    }

    static Block* endRequest(uint32_t status, uint16_t id, uint8_t proto)
    {
        Block* block=new Block(END_REQUEST, id, sizeof(EndRequestRecord));
        EndRequestRecord* request=(EndRequestRecord*)(block->_data.c_str());
        request->status(status);
        request->protoStatus(proto);
        return block;
    }

    boost::shared_ptr<Transceiver> _tr;
    RecordHeader               _header;
    std::string                  _data;
};
#endif


#endif
