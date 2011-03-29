#include <string>
#include <ostream>
#include <sstream>
#include <iostream>

#include "Request.hpp"

namespace fcgi {

    const std::string fcgiMaxConns  = FCGI_MAX_CONNS;
    const std::string fcgiMaxReqs   = FCGI_MAX_REQS;
    const std::string fcgiMpxsConns = FCGI_MPXS_CONNS;

    void RequestBase::requestComplete(uint32_t status)
    {
        flushStreams();
        _tr->requestComplete(status, getId(), isKeepConnection());
    }

    void RequestBase::flush(boost::shared_ptr<Block> &blk)
    {
        if (blk.get() && blk->getRecordHeader()->contentLength()>0) {
            _tr->writeBlock(blk);
            blk.reset();
        }
    }

    void RequestBase::flushStreams()
    {
        _ostream.flush();
        _estream.flush();
        flush(_outStreamBlk);
        flush(_errStreamBlk);
    }

    size_t getNVLength(const char* data, uint32_t& len) {
        uint8_t b = (uint8_t)data[0];
        if ((b & 0x80)==0) {
            len  = (uint32_t)b;
            return 1;
        }
        else {
            len = 0x7fffffff & ntohl(*((uint32_t*)&(data[0])));
            return 4;
        }
    }

}
