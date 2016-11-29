#include "graph_dummy.hh"
#include "helpers.hh"

#include <iostream>

Graph_Dummy::Graph_Dummy(string& auditfile) : data(auditfile) {
}

vector<Node_Id> Graph_Dummy::get_edges(Node_Id node, size_t pos,
        size_t nbits_degree, size_t nbits_delta) {
}

vector<Node_Id> Graph_Dummy::get_outgoing_edges(Node_Id node) {
}

vector<Node_Id> Graph_Dummy::get_incoming_edges(Node_Id node) {
}

size_t Graph_Dummy::get_node_count() {
    return 0;
}

Graph_Dummy::~Graph_Dummy() {}
