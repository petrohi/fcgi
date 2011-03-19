#ifndef _FCGI_Block_h_
#define _FCGI_Block_h_

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

        Block(RecordType type, uint16_t id, uint16_t len) :
            _data(len+sizeof(RecordHeader), 0)
        {
            getRecordHeader()->init(fcgiProtoVersion, type, id, len);
        }

        RecordHeader* getRecordHeader() const {
            if (_header.size()==0)
                return (RecordHeader*)(_data.data());
            else
                return (RecordHeader*)(_header.data());
        }

        EndRequestRecord* getEndRequestRecord() const {
            return (EndRequestRecord*)(_data.c_str() + sizeof(RecordHeader));
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
            EndRequestRecord* request=block->getEndRequestRecord();
            request->status(status);
            request->protoStatus(proto);
            return block;
        }

        bool isOnePiece() const { return _header.empty(); }

        const std::string& getData() const { return _data; }
        const std::string& getHeader() const { return _header; }

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
