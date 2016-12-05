#include "graph_v2.hh"

#include <iostream>

using namespace std;

Graph_V2::Graph_V2(string& compressed) : data(compressed) {
    info_t fwd_info_c, fwd_info_notc, back_info_c, back_info_notc;
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
        cout << "FWDEDGES" << endl;
        for (Node_Id id : get_outgoing_edges_raw(idx)) {
            cout << id << endl;
        }
        cout << "BACKEDGES" << endl;
        for (Node_Id id : get_incoming_edges_raw(idx)) {
            cout << id << endl;
        }
        cout << endl;
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
    return idx2pos[idx] + base_pos;
}

vector<Node_Id> Graph_V2::read_edges_raw(Group_Idx idx, size_t pos,
        info_t info) {
    size_t degree;
    pos += data.get_bits<size_t>(degree, info.nbits_degree, pos);
    if (!degree) {
        return {};
    }
    Node_Id base_node = get_group_id(idx);
    size_t first;
    pos += data.get_bits<size_t>(first, info.nbits_delta + 1, pos);
    vector<Node_Id> edges;
    if (first % 2) {
        first = base_node - (first - 1) / 2;
    } else {
        first = base_node + first / 2;
    }
    edges.push_back(first);
    for (size_t i = 1; i < degree; ++i) {
        size_t delta;
        pos += data.get_bits<size_t>(delta, info.nbits_delta, pos);
        edges.push_back(edges.back() + delta);
    }
    return edges;
}

vector<Node_Id> Graph_V2::get_outgoing_edges_raw(Group_Idx idx) {
    size_t pos = get_group_pos(idx);
    info_t info = fwd_info[get_group_size(idx) > 1];
    return read_edges_raw(idx, pos, info);
}

vector<Node_Id> Graph_V2::get_incoming_edges_raw(Group_Idx idx) {
    size_t pos = get_group_pos(idx);
    bool is_collapsed = get_group_size(idx) > 1;
    info_t info = fwd_info[is_collapsed];
    size_t degree;
    pos += data.get_bits<size_t>(degree, info.nbits_degree, pos);
    if (degree) {
        pos += info.nbits_delta + 1;
        pos += (degree - 1) * info.nbits_delta;
    }
    return read_edges_raw(idx, pos, back_info[is_collapsed]);
}

vector<Node_Id> Graph_V2::get_edges(Node_Id, bool,
        vector<Node_Id> (Graph_V2::*)(Group_Idx),
        vector<Node_Id> (Graph_V2::*)(Group_Idx)) {
    return {};
}
