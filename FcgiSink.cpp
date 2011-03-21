#include <boost/noncopyable.hpp>

#include "FcgiSink.hpp"
#include "Block.hpp"
#include "Request.hpp"

#include <string.h>

namespace fcgi {

    const std::streamsize FcgiSink::_size = 0xffda;
    
    inline
    boost::shared_ptr<Block>& FcgiSink::blk()
    {
        return (_out ? _rq._outStreamBlk : _rq._errStreamBlk);
    }

    std::streamsize FcgiSink::write(const char_type* src, std::streamsize n)
    {
        std::string test(src, n);
        std::cout << "[" << n <<"] "<< std::endl;

        if (!blk().get())
            newBlock();

        std::streamsize dataToWrite=n;

        while (dataToWrite>0) {
            size_t spaceToWrite=_size-blk()->data().size();
            if (dataToWrite<spaceToWrite) {
                blk()->data().append(src,dataToWrite);
                blk()->getRecordHeader()->contentLength(blk()->data().size()-sizeof(RecordHeader));
                dataToWrite=0;
            }
            else {
                blk()->data().append(src, spaceToWrite);
                dataToWrite-=spaceToWrite;
                src+=spaceToWrite;
                blk()->getRecordHeader()->contentLength(blk()->data().size()-sizeof(RecordHeader));
                _rq._tr->writeBlock(blk());
                newBlock();
            }
        }
        return n;
    }

    void FcgiSink::newBlock()
    {
        blk().reset(Block::stream(_rq.getRequestId(), _out, _size));
    }

}
