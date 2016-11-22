CC = gcc
CCFLAGS =
CXX = g++
CXXFLAGS = -std=gnu++0x -g 
OPTFLAGS = -W -Wall -O3
OBJS = helpers.o queriers.o metadata.o clp.o jsoncpp.o
DEPS = $(OBJS)

%.o: %.c
	$(CC) $(CCFLAGS) $(OPTFLAGS) -c -o $@ $<

%.o: %.cc 
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -c -o $@ $<

query: query.o $(DEPS)
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -o $@ $< $(OBJS)

graph:
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -o $@ graph.cc graphv1.cc helpers.cc

clean:
	rm -f *.o graph query
