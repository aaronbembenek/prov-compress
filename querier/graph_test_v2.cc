#include "graph_v2.hh"

#include <iostream>

using namespace std;

int main() {
    string buffer;
    read_file("../compression/trial.cpg2", buffer);

    Graph* graph = new Graph_V2(buffer);
    (void) graph;
}
