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

    void RequestBase::flushStreams()
    {
        if (!_ostream.str().empty()) {
            std::string buf(_ostream.str());
            _tr->writeStream(getRequestId(), buf, true);
        }
        if (!_estream.str().empty()) {
            std::string buf(_estream.str());
            _tr->writeStream(getRequestId(), buf, false);
        }
    }

    size_t getNVLength(const char* data, uint32_t& len) {
        uint8_t b = (uint8_t)data[0];
        if ((b & 0x80)==0) {
            len  = (uint32_t)b;
            return 1;
        }
        else {
            len = ntohl(*((uint32_t*)data));
            return 4;
        }
    }

}
