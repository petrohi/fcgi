#ifndef _FCGI_Exceptions_h_
#define _FCGI_Exceptions_h_

#include <exception>

namespace fcgi
{
    namespace Exceptions
    {
        struct FcgiException : public std::exception
        {
            FcgiException(const char* msg, const int erno) : _errno(erno), _msg(msg) {}
            const char* what() const throw() { return _msg; }
            const int _errno;
            const char* _msg;
        };

        struct Socket: public FcgiException
        {
            Socket(const int& fd, const int& erno)
                : FcgiException(0, erno), _fd(fd) {}
            const int _fd;
        };

        struct SocketWrite : public Socket
        {
            SocketWrite(int fd, int erno);
        };
        
        struct SocketRead: public Socket
        {
            SocketRead(int fd, int erno);
        };

        struct SocketAccept: public Socket
        {
            SocketAccept(int fd, int erno);
        };

    }
}

#endif
