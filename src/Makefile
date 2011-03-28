EXAMPLE = async
FCGILIB = libfcgi.a
OBJS =  main.o
LIBOBJS = Exceptions.o Transceiver.o Request.o FcgiSink.o Http.o 
INCLUDES = -I../include -I..

HEADERS = fcgi.hpp Block.hpp Exceptions.hpp FcgiSink.hpp \
	Http.hpp Manager.hpp Protocol.hpp Request.hpp Transceiver.hpp

PREFIX = /usr/local
INSTALL_INC = $(PREFIX)/include/fcgi
INSTALL_LIB = $(PREFIX)/lib
INSTALL = install -m 0644

all: .depend $(FCGILIB)

$(EXAMPLE): $(OBJS) $(FCGILIB)
	$(CXX) -o async $(OBJS) $(FCGILIB) -lhiredis -lboost_wserialization /usr/local/lib/libev.a

$(FCGILIB): $(LIBOBJS)
	$(AR) cr $(FCGILIB) $(LIBOBJS)

%.o: %.cpp
	$(CXX) $(INCLUDES) -g -c $< 

clean:
	rm -f *.o

.depend dep:
	$(CXX) $(INCLUDES) -MM *.cpp > .depend

install: ${FCGILIB}
	mkdir -p $(INSTALL_INC) $(INSTALL_LIB)
	cd ../include ; $(INSTALL) $(HEADERS) $(INSTALL_INC) 
	cd ../src ;	$(INSTALL) ${FCGILIB} $(INSTALL_LIB)

include .depend
