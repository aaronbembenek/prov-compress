CC = gcc
CXX = g++
CXXFLAGS =-std=gnu++0x -g -W -Wall -O3
DEPS = "helpers.hh" "query.hh"
OBJ = helpers.o metadata.o query.o

%.o: %.c $(DEPS)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

query: $(OBJ)
	gcc -o $@ $^ $(CXXFLAGS)

clean:
	rm -f query.o
