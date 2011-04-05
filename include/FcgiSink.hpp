#ifndef _FcgiSink_h_
#define _FcgiSink_h_

#include <boost/shared_ptr.hpp>

#include <boost/iostreams/detail/ios.hpp> // streamsize.
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/categories.hpp>

namespace fcgi {

    class Block;
    class RequestBase;
    class FcgiSink 
    {
    public:
        typedef char                         char_type;
        typedef boost::iostreams::sink_tag    category;

        FcgiSink(RequestBase& rq, bool isOut)
            : _rq(rq), _out(isOut)
        {}

        inline
        boost::shared_ptr<Block>& blk();

        std::streamsize write(const char_type* src, std::streamsize n);

        void newBlock();

    private:
        RequestBase&              _rq;
        bool                      _out;
        static const std::streamsize _size;
    };

    class ConverterSink 
    {
        ConverterSink();
    public:
        typedef wchar_t                      char_type;
        typedef boost::iostreams::sink_tag    category;

        ConverterSink(RequestBase& rq)
            : _rq(rq)
        {}

        std::streamsize write(const char_type* src, std::streamsize n);

    private:
        RequestBase& _rq;
    };

}

#endif
