EXAMPLE = async
FCGILIB = libfcgi.a
OBJS =  main.o
LIBOBJS = Exceptions.o Transceiver.o Request.o FcgiSink.o Http.o 

all: $(EXAMPLE) $(FCGILIB)

$(EXAMPLE): $(OBJS) $(FCGILIB)
	$(CXX) -o async $(OBJS) $(FCGILIB) -lhiredis -lboost_wserialization /usr/local/lib/libev.a

$(FCGILIB): $(LIBOBJS)
	$(AR) cr $(FCGILIB) $(LIBOBJS)

%.o: %.cpp
	$(CXX) -g -c $< 

clean:
	rm -f *.o

Request.o: Request.cpp Request.hpp Transceiver.hpp Protocol.hpp \
 Exceptions.hpp Block.hpp FcgiSink.hpp
main.o: main.cpp ../hiredispp/hiredispp.h ../hiredispp/hiredispp_a.h \
 Manager.hpp Transceiver.hpp Protocol.hpp Exceptions.hpp Block.hpp \
 Request.hpp FcgiSink.hpp AppHandler.hpp Http.hpp
Exceptions.o: Exceptions.cpp Exceptions.hpp
FcgiSink.o: FcgiSink.cpp FcgiSink.hpp Block.hpp Protocol.hpp Request.hpp \
 Transceiver.hpp Exceptions.hpp
main.o: main.cpp ../hiredispp/hiredispp.h ../hiredispp/hiredispp_a.h \
 Manager.hpp Transceiver.hpp Protocol.hpp Exceptions.hpp Block.hpp \
 Request.hpp FcgiSink.hpp AppHandler.hpp
Request.o: Request.cpp Request.hpp Transceiver.hpp Protocol.hpp \
 Exceptions.hpp Block.hpp FcgiSink.hpp
Transceiver.o: Transceiver.cpp Transceiver.hpp Protocol.hpp \
 Exceptions.hpp Block.hpp Manager.hpp Request.hpp FcgiSink.hpp
Http.o: Http.cpp
