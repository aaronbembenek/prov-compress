#include "graph.hh"
#include "graph_v1.hh"
#include "helpers.hh"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    string buffer;
    if (argc < 2) {
        read_file("samples/tiny_dag.cpg", buffer);
    } else {
        read_file(argv[1], buffer);
    }
    Graph* graph = new Graph_V1(buffer);
    size_t n = graph->get_node_count();
    for (Node_Id i = 0; i < n; ++i) {
        cout << "Node " << i << "'s children:" << endl;
        for (auto child : graph->get_outgoing_edges(i)) {
            cout << child << endl;
        }
    }
    for (Node_Id i = 0; i < n; ++i) {
        cout << "Descendants of " << i << ":" << endl;
        for (auto descendant : graph->get_all_descendants(i)) {
            cout << descendant << endl;
        }
    }
}
