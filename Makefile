CC = gcc
CXX = g++
CXXFLAGS =-std=gnu++0x -g
OPTFLAGS = -W -Wall -O3
OBJS = helpers.o queriers.o metadata.o
DEPS = $(OBJS) 

%.o: %.cc 
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -c -o $@ $<

query: query.o $(DEPS)
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -o $@ $< $(OBJS)

clean:
	rm -f queriers.o metadata.o query.o helpers.o query
