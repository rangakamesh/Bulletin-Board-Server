CXX = g++
CXXFLAGS = -g -Werror -ansi -pedantic --std=c++11
LDFLAGS = $(CXXFLAGS) -pthread

all: bbserv

tcp-utils.o: tcp-utils.h tcp-utils.cc
	$(CXX) $(CXXFLAGS) -c -o tcp-utils.o tcp-utils.cc
tokenize.o: tokenize.h
	$(CXX) $(CXXFLAGS) -c -o tokenize.o tokenize.cc

bbserv_utils.o: bbserv_utils.h bbserv_utils.cc
	$(CXX) $(CXXFLAGS) -c -o bbserv_utils.o bbserv_utils.cc

descriptor.o: descriptor.h descriptor.cc
	$(CXX) $(CXXFLAGS) -c -o descriptor.o descriptor.cc

thrd_mgmt.o: thrd_mgmt.h thrd_mgmt.cc
	$(CXX) $(CXXFLAGS) -c -o thrd_mgmt.o thrd_mgmt.cc

server_operations.o: server_operations.h server_operations.cc
	$(CXX) $(CXXFLAGS) -c -o server_operations.o server_operations.cc

peer_operations.o: peer_operations.h peer_operations.cc
	$(CXX) $(CXXFLAGS) -c -o peer_operations.o peer_operations.cc

bbserv: tokenize.o tcp-utils.o bbserv_utils.o thrd_mgmt.o  server_operations.o descriptor.o peer_operations.o
	$(CXX) $(LDFLAGS) -o bbserv bbserv.cc tokenize.o tcp-utils.o bbserv_utils.o thrd_mgmt.o  server_operations.o descriptor.o peer_operations.o

clean:
	rm -f *~ *.o *.bak core \#*

distclean: clean
	rm -f bbserv test filetest *.log *.pid
