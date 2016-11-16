CC = gcc
CXX = g++
CXXFLAGS =-std=gnu++0x -g
OPTFLAGS = -W -Wall -O3

all:
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) query.cc helpers.cc -o query.o 

clean:
	rm -f query.o
