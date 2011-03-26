EXAMPLE = async
FCGILIB = libfcgi.a
OBJS =  main.o
LIBOBJS = Exceptions.o Transceiver.o Request.o FcgiSink.o Http.o 
INCLUDES = -I../include -I..

all: .depend $(EXAMPLE) $(FCGILIB)

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

include .depend
