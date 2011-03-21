OBJS = Exceptions.o Transceiver.o Request.o main.o

async: $(OBJS)
	$(CXX) -o async $(OBJS) -lhiredis /usr/local/lib/libev.a 

%.o: %.cpp
	$(CXX) -g -c $< 

main.o: main.cpp ../hiredispp/hiredispp.h ../hiredispp/hiredispp_a.h \
 Manager.hpp Transceiver.hpp Protocol.hpp Exceptions.hpp Request.hpp \
 AppHandler.hpp Transceiver.cpp Request.cpp Block.hpp

Transceiver.o: Transceiver.cpp Transceiver.hpp