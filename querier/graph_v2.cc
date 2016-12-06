#include "graph_v2.hh"

#include <iostream>

using namespace std;

Graph_V2::Graph_V2(string& compressed) : data(compressed) {
    info fwd_info_c, fwd_info_notc, back_info_c, back_info_notc;
    int pos = 0;
    pos += data.get_bits<size_t>(fwd_info_c.nbits_degree, 8, pos);
    pos += data.get_bits<size_t>(fwd_info_c.nbits_delta, 8, pos);
    pos += data.get_bits<size_t>(fwd_info_notc.nbits_degree, 8, pos);
    pos += data.get_bits<size_t>(fwd_info_notc.nbits_delta, 8, pos);
    pos += data.get_bits<size_t>(back_info_c.nbits_degree, 8, pos);
    pos += data.get_bits<size_t>(back_info_c.nbits_delta, 8, pos);
    pos += data.get_bits<size_t>(back_info_notc.nbits_degree, 8, pos);
    pos += data.get_bits<size_t>(back_info_notc.nbits_delta, 8, pos);
    fwd_info[true] = fwd_info_c;
    fwd_info[false] = fwd_info_notc;
    back_info[true] = back_info_c;
    back_info[false] = back_info_notc;

    size_t nbits_size_entry, nbits_index_entry;
    pos += data.get_bits<size_t>(nbits_size_entry, 8, pos);
    pos += data.get_bits<size_t>(nbits_index_entry, 8, pos);
    pos += data.get_bits<size_t>(group_index_length, 32, pos);
    ++group_index_length;
    group_index = new Node_Id[group_index_length];
    idx2pos = new size_t[group_index_length - 1];
    size_t prev_size = 0;
    Node_Id prev_id = 0;
    size_t prev_loc = 0;
    for (size_t i = 0; i < group_index_length - 1; ++i) {
        size_t sz, loc;
        pos += data.get_bits<size_t>(loc, nbits_index_entry, pos);
        pos += data.get_bits<size_t>(sz, nbits_size_entry, pos);
        Node_Id id = prev_id + prev_size;
        loc += prev_loc;
        group_index[i] = id;
        idx2pos[i] = loc;
        prev_loc = loc;
        prev_id = id;
        prev_size = sz;
    }
    group_index[group_index_length - 1] = prev_id + prev_size;
    base_pos = ((pos + 7) >> 3) << 3;

    for (Node_Id i = 0; i < get_node_count(); ++i) {
        Group_Idx idx = get_group_index(i);
        size_t sz = get_group_size(idx);
        size_t pos = get_group_pos(idx);
        cout << i << " " << idx << " " << sz << " " << pos << endl;
    }
}

Graph_V2::~Graph_V2() {
    delete [] group_index;
    delete [] idx2pos;
}

vector<Node_Id> Graph_V2::get_outgoing_edges(Node_Id) {
    return {};
}

vector<Node_Id> Graph_V2::get_incoming_edges(Node_Id) {
    return {};
}

size_t Graph_V2::get_node_count() {
    return group_index[group_index_length - 1];
}

// XXX This should use binary search.
Group_Idx Graph_V2::get_group_index(Node_Id node) {
    size_t i;
    for (i = group_index_length - 2; i > 0; --i) {
        if (group_index[i] <= node) {
            break;
        }
    }
    return (Group_Idx) i;
}

size_t Graph_V2::get_group_size(Group_Idx idx) {
    return group_index[idx + 1] - group_index[idx]; 
}

Node_Id Graph_V2::get_group_id(Group_Idx idx) {
    return group_index[idx];
}

size_t Graph_V2::get_group_pos(Group_Idx idx) {
    return idx2pos[idx];
}

vector<Node_Id> Graph_V2::read_edges_raw(Group_Idx, size_t, info) {
    return {};
}

vector<Node_Id> Graph_V2::get_outgoing_edges_raw(Group_Idx) {
    return {};
}

vector<Node_Id> Graph_V2::get_incoming_edges_raw(Group_Idx) {
    return {};
}

vector<Node_Id> Graph_V2::get_edges(Node_Id, bool,
        vector<Node_Id> (Graph_V2::*)(Group_Idx),
        vector<Node_Id> (Graph_V2::*)(Group_Idx)) {
    return {};
}
