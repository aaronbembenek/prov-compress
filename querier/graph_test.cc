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
        cout << "Reading from " << argv[1] << endl;
        read_file(argv[1], buffer);
    }
    Graph* graph = new Graph_V1(buffer);
    size_t n = graph->get_node_count();
    for (Node_Id i = 0; i < n; ++i) {
        cout << "NODE " << i << endl;
        cout << "Children:" << endl;
        for (auto child : graph->get_outgoing_edges(i)) {
            cout << child << endl;
        }
        cout << "Descendants:" << endl;
        for (auto descendant : graph->get_all_descendants(i)) {
            cout << descendant << endl;
        }
        cout << "Parents:" << endl;
        for (auto child : graph->get_incoming_edges(i)) {
            cout << child << endl;
        }
        cout << "Ancestors:" << endl;
        for (auto child : graph->get_all_ancestors(i)) {
            cout << child << endl;
        }
        cout << endl;
    }
}
