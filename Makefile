OBJS = Exceptions.o Transceiver.o Request.o FcgiSink.o main.o

async: $(OBJS)
	$(CXX) -o async $(OBJS) -lhiredis /usr/local/lib/libev.a 

%.o: %.cpp
	$(CXX) -g -c $< 

Request.o: Request.cpp Request.hpp Transceiver.hpp Protocol.hpp \
 Exceptions.hpp Block.hpp FcgiSink.hpp
main.o: main.cpp ../hiredispp/hiredispp.h ../hiredispp/hiredispp_a.h \
 Manager.hpp Transceiver.hpp Protocol.hpp Exceptions.hpp Block.hpp \
 Request.hpp FcgiSink.hpp AppHandler.hpp
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
