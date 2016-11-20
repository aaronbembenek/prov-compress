CC = gcc
CCFLAGS =
CXX = g++
CXXFLAGS = -std=gnu++0x -g 
OPTFLAGS = -W -Wall -O3
OBJS = helpers.o queriers.o metadata.o clp.o
DEPS = $(OBJS)

%.o: %.c
	$(CC) $(CCFLAGS) $(OPTFLAGS) -c -o $@ $<

%.o: %.cc 
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -c -o $@ $<

query: query.o $(DEPS)
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -o $@ $< $(OBJS)

clean:
	rm -f queriers.o metadata.o query.o helpers.o query clp.o
