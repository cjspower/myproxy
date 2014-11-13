# Makefile for ECEN602, HW3 Proxy server

CXX      = /usr/bin/gcc
all:proxy client
proxy: 
	$(CXX) $(CXXFLAGS) -o proxy proxy.c -lpthread
client:
	$(CXX) $(CXXFLAGS) -o client client.c

clean:
	@rm proxy
	@rm client
