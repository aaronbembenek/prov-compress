CC = gcc
CCFLAGS =
CXX = g++
ifeq ($(COMPRESSED), 1)
CXXFLAGS = -std=gnu++0x -g -DCOMPRESSED
else
CXXFLAGS = -std=gnu++0x -g
endif
OPTFLAGS = -W -Wall -O3 -DBESAFE
OBJS = helpers.o metadata_compressed.o clp.o jsoncpp.o graph.o graph_v1.o json_graph.o queriers.o 
DEPS = $(OBJS)

%.o: %.c
	$(CC) $(CCFLAGS) $(OPTFLAGS) -c -o $@ $<

%.o: %.cc 
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -c -o $@ $<

query: query.o $(DEPS)
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -o $@ $< $(OBJS)

graph: graph_test_v2.o graph.o graph_v1.o helpers.o json_graph.o\
	metadata_compressed.o jsoncpp.o graph_v2.o
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -o $@ $^

clean:
	rm -f *.o graph query dummy_query
