#include "graph_v1.hh"
#include "helpers.hh"

#include <iostream>

Graph_V1::Graph_V1(string& compressed) : data(compressed) {
    // First 32 bits have basic metadata.
    int pos = 0;
    pos += data.get_bits<size_t>(nbits_degree, 8, pos);
    pos += data.get_bits<size_t>(nbits_delta, 8, pos);
    size_t nbits_index_entry;
    pos += data.get_bits<size_t>(nbits_index_entry, 8, pos);
    pos += data.get_bits<size_t>(index_length, 8, pos);
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
    cout << "nbits_degree: " << nbits_degree << endl;
    cout << "nbits_delta: " << nbits_delta << endl;
    cout << "nbits_index_entry: " << nbits_index_entry << endl;
    cout << "index_length: " << index_length << endl;
    for (int i = 0; i < index_length; ++i) {
        cout << index[i] << endl;
    }
    */
}

vector<Node_Id> Graph_V1::get_outgoing_edges(Node_Id node) {
    assert(node < index_length);
    size_t pos = index[node] + base_pos;
    size_t outdegree;
    pos += data.get_bits<size_t>(outdegree, nbits_degree, pos);
    if (!outdegree) {
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
    for (size_t i = 1; i < outdegree; ++i) {
        Node_Id child;
        pos += data.get_bits<Node_Id>(child, nbits_delta, pos);
        children.push_back(children.back() + child);
    }
    return children;
}

size_t Graph_V1::get_node_count() {
    return index_length;
}

Graph_V1::~Graph_V1() {
    delete [] this->index;
}
