CC = gcc
CCFLAGS =
CXX = g++
CXXFLAGS = -std=gnu++0x -g 
OPTFLAGS = -W -Wall -O3
OBJS = helpers.o queriers.o metadata.o clp.o jsoncpp.o graph.o graph_v1.o graph_dummy.o
DEPS = $(OBJS)

%.o: %.c
	$(CC) $(CCFLAGS) $(OPTFLAGS) -c -o $@ $<

%.o: %.cc 
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -c -o $@ $<

query: query.o $(DEPS)
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -o $@ $< $(OBJS)

graph: graph_test.o graph.o graph_v1.o helpers.o graph_dummy.o
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -o $@ $^ 

clean:
	rm -f *.o graph query
