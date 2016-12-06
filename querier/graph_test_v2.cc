#include "graph_v2.hh"

#include <iostream>

using namespace std;

int main() {
    string buffer;
    read_file("../compression/trial.cpg2", buffer);

    Graph* graph = new Graph_V2(buffer);
    for (Node_Id node = 0; node < graph->get_node_count(); ++node) {
        cout << "NODE " << node << endl;
        cout << "OUTGOING " << endl;
        for (Node_Id e : graph->get_outgoing_edges(node)) {
            cout << e << endl;
        }
        cout << "INCOMING " << endl;
        for (Node_Id e : graph->get_incoming_edges(node)) {
            cout << e << endl;
        }
        cout << endl;
    }
}
