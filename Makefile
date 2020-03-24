SET(CMAKE_CXX_FLAGS "-std=c++11 -O3")
CC	 	= g++
LD	 	= g++
CFLAGS	 	= -Wall -g
LDFLAGS	 	= 
DEFS 	 	=

all: sendfile recvfile

server: sendfile.cpp
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o sendfile sendfile.cpp utils.h

client: recvfile.cpp
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o recvfile recvfile.cpp utils.h

clean:
	rm -f *.o
	rm -f *~
	rm -f core.*.*
	rm -f server
	rm -f client