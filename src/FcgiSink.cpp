#include <boost/noncopyable.hpp>
// #include <string.h>

#include "Protocol.hpp"
#include "Block.hpp"
#include "FcgiSink.hpp"
#include "Transceiver.hpp"
#include "Request.hpp"
#include "Http.hpp"

using namespace std;

namespace fcgi {

    const std::streamsize FcgiSink::_size = 65480; // 0xffda;
    
    inline
    boost::shared_ptr<Block>& FcgiSink::blk()
    {
        return (_out ? _rq._outStreamBlk : _rq._errStreamBlk);
    }

    std::streamsize FcgiSink::write(const char_type* src, std::streamsize n)
    {
        if (!blk().get())
            newBlock();

        std::streamsize size = blk()->data().capacity()-1;

        if (size>0xffff) {
#ifdef FCGI_DEBUG
            std::cout << "[" << n <<"] size="<<blk()->data().size()
                      << " capacity=" << blk()->data().capacity()<< std::endl;
#endif
            size=0xfff0;
        }

        std::streamsize dataToWrite=n;

        while (dataToWrite>0) {
            size_t spaceToWrite=size-blk()->data().size();
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
                // std::cout << "[" << n <<"] size="<<blk()->data().size()
                //          << " capacity=" << blk()->data().capacity()<< std::endl;
                _rq._tr->writeBlock(blk());
                newBlock();
            }
        }
        return n;
    }

    void FcgiSink::newBlock()
    {
        blk().reset(Block::stream(_rq.getId(), _out, _size));
    }

    std::streamsize ConverterSink::write(const char_type* src, std::streamsize n)
    {
        std::wstring str(src,n);
        Http::HttpString<wchar_t>::encode(str,
                                          static_cast<std::ostream&>(_rq._ostream));
        return n;
    }

}
