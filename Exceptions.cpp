#include <errno.h>
#include "Exceptions.hpp"

namespace fcgi {
    namespace Exceptions {

        SocketRead::SocketRead(int fd, int erno)
            : Socket(fd, erno)
        {
            switch(erno) {
            case EAGAIN:
                _msg = "Non-blocking I/O has been selected using "
                    "O_NONBLOCK and no data was immediately available for reading.";
                break;
            case EBADF:
                _msg = "The file descriptor is not valid or is not open for reading.";
                break;
            case EFAULT:
                _msg = "The buffer is outside your accessible address space.";
                break;
            case EINTR:
                _msg = "The call was interrupted by a signal before "
                    "any data was written; see signal(7).";
                break;
            case EINVAL:
                _msg = "The file descriptor is attached to an object which is unsuitable "
                    "for reading; or the file was opened with the O_DIRECT flag, "
                    "and either the address specified in buf, the value specified in "
                    "count, or the current file offset is not suitably aligned.";
                break;
            case EIO:
                _msg = "I/O error.  This will happen for example when the process is "
                    "in a background process group, tries to read from its controlling "
                    "tty, and either it is ignoring or blocking SIGTTIN or its process "
                    "group is orphaned.  It may also occur when there is a low-level I/O "
                    "error while reading from a disk or tape.";
                break;
            case EISDIR:
                _msg = "The file descriptor refers to a directory.";
                break;
            case EINPROGRESS:
                _msg = "Operation now in progress.";
                break;
            default:
                _msg = "UNKNOWN ERROR";
                break;
            }
        }
    
        SocketWrite::SocketWrite(int fd, int erno) 
            : Socket(fd, erno)
        {
            switch(erno) {
            case EAGAIN:
                _msg = "The file descriptor has been marked non-blocking (O_NONBLOCK) and the write would block.";
                break;
            case EBADF:
                _msg = "The file descriptor is not a valid file descriptor or is not open for writing.";
                break;
            case EFAULT:
                _msg = "The buffer is outside your accessible address space.";
                break;
            case EFBIG:
                _msg = "An attempt was made to write a file that exceeds the implementation-defined maximum file size or the process’s file size limit, or to write at a position past the maximum allowed offset.";
                break;
            case EINTR:
                _msg = "The call was interrupted by a signal before any data was written; see signal(7).";
                break;
            case EINVAL:
                _msg = "The file descriptor is attached to an object which is unsuitable for writing; or the file was opened with the O_DIRECT flag, and either the address specified for the buffer, the value specified in count, or the current file offset is not suitably aligned.";
                break;
            case EIO:
                _msg = "A low-level I/O error occurred while modifying the inode.";
                break;
            case ENOSPC:
                _msg = "The device containing the file referred to by the file descriptor has no room for the data.";
                break;
            case EPIPE:
                _msg = "The file descriptor is connected to a pipe or socket whose reading end is closed.  When this happens the writing process will also receive a SIGPIPE signal.  (Thus, the write return value is seen only if the program catches, blocks or ignores this signal.)";
                break;
            default:
                _msg = "UNKNOWN ERROR";
            }
        }
        SocketAccept::SocketAccept(int fd, int erno)
            : Socket(fd, erno)
        {
            switch(erno) {
            case EBADF: _msg ="The descriptor is invalid.";
                break;
            case ECONNABORTED: _msg = "A connection has been aborted.";
                break;
            case EFAULT: _msg="The addr argument is not in a writable part of the user address space.";
                break;
            case EINTR: _msg="The system call was interrupted by a signal that "
                    "was caught before a valid connection arrived; see signal(7)."; 
                break;
            case EINVAL: _msg="Socket is not listening for connections, "
                    "or addrlen is invalid (e.g., is negative).";
                break;
            case EMFILE: _msg="The per-process limit of open file descriptors has been reached.";
                break;
            case ENFILE: _msg="The system limit on the total number of open files has been reached.";
                break;
            case ENOBUFS:
            case ENOMEM:
                _msg="Not enough free memory. "
                    "This often means that the memory allocation is limited by "
                    "the socket buffer limits, not by the system memory.";
                break;
            case ENOTSOCK:
                _msg="The descriptor references a file, not a socket.";
                break;
            default:
                _msg = "UNKNOWN ERROR";
                break;
            }
        }

    }}
