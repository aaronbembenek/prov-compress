#include "graph_v1.hh"
#include "helpers.hh"

#include <iostream>

Graph_V1::Graph_V1(string& compressed) : data(compressed) {
    // First 32 bits have basic metadata.
    int pos = 0;
    pos += data.get_bits<size_t>(nbits_outdegree, 8, pos);
    pos += data.get_bits<size_t>(nbits_outdelta, 8, pos);
    pos += data.get_bits<size_t>(nbits_indegree, 8, pos);
    pos += data.get_bits<size_t>(nbits_indelta, 8, pos);
    size_t nbits_index_entry;
    pos += data.get_bits<size_t>(nbits_index_entry, 8, pos);
    pos += data.get_bits<size_t>(index_length, 32, pos);
    // FIXME potential overflow here...
    assert(index_length != (size_t) -1);
    index_length += 1;

    // Create index from node ID into node data.
    index = new size_t[index_length];
    index[0] = 0;
    for (size_t i = 1; i < index_length; ++i) {
        pos += data.get_bits<size_t>(index[i], nbits_index_entry, pos);
        index[i] += index[i - 1];
    }
    base_pos = ((pos + 7) >> 3) << 3;

    /*    
    cout << "nbits_outdegree: " << nbits_outdegree << endl;
    cout << "nbits_outdelta: " << nbits_outdelta << endl;
    cout << "nbits_indegree: " << nbits_indegree << endl;
    cout << "nbits_indelta: " << nbits_indelta << endl;
    cout << "nbits_index_entry: " << nbits_index_entry << endl;
    cout << "index_length: " << index_length << endl;
    for (size_t i = 0; i < index_length; ++i) {
        cout << index[i] << endl;
    }
    */
}

vector<Node_Id> Graph_V1::get_edges(Node_Id node, size_t pos,
        size_t nbits_degree, size_t nbits_delta) {
    size_t degree;
    pos += data.get_bits<size_t>(degree, nbits_degree, pos);
    if (!degree) {
        return {};
    }
    vector<Node_Id> children;
    Node_Id first;
    pos += data.get_bits<Node_Id>(first, nbits_delta + 1, pos);
    if (first % 2) {
        first = node - (first - 1) / 2;
    } else {
        first = node + first / 2;
    }
    children.push_back(first);
    for (size_t i = 1; i < degree; ++i) {
        Node_Id child;
        pos += data.get_bits<Node_Id>(child, nbits_delta, pos);
        children.push_back(children.back() + child);
    }
    return children;
}

vector<Node_Id> Graph_V1::get_outgoing_edges(Node_Id node) {
    return get_edges(node, index[node] + base_pos, nbits_outdegree,
            nbits_outdelta);
}

vector<Node_Id> Graph_V1::get_incoming_edges(Node_Id node) {
    size_t pos = index[node] + base_pos;
    size_t degree;
    pos += data.get_bits<size_t>(degree, nbits_outdegree, pos);
    if (degree) {
        pos += nbits_outdelta + 1;
        pos += (degree - 1) * nbits_outdelta;
    }
    return get_edges(node, pos, nbits_indegree, nbits_indelta);
}

size_t Graph_V1::get_node_count() {
    return index_length;
}

Graph_V1::~Graph_V1() {
    delete [] this->index;
}
