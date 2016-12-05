#include "graph_v2.hh"

using namespace std;


Graph_V2::Graph_V2(string& compressed) : data(compressed) {
}

Graph_V2::~Graph_V2() {
    delete [] group_index;
}

vector<Node_Id> Graph_V2::get_outgoing_edges(Node_Id) {
    return {};
}

vector<Node_Id> Graph_V2::get_incoming_edges(Node_Id) {
    return {};
}

size_t Graph_V2::get_node_count() {
    return 0;
}

group_t Graph_V2::get_group(Node_Id) {
    return (group_t) 0;
}

size_t Graph_V2::get_group_size(group_t) {
    return 0;
}

Node_Id Graph_V2::get_group_id(group_t) {
    return 0;
}

vector<Node_Id> Graph_V2::read_edges_raw(group_t, size_t, info) {
    return {};
}

vector<Node_Id> Graph_V2::get_outgoing_edges_raw(group_t) {
    return {};
}

vector<Node_Id> Graph_V2::get_incoming_edges_raw(group_t) {
    return {};
}

vector<Node_Id> Graph_V2::get_edges(Node_Id, bool,
        vector<Node_Id> (Graph_V2::*)(group_t),
        vector<Node_Id> (Graph_V2::*)(group_t)) {
    return {};
}
