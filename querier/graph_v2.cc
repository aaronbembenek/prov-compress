#include "graph_v2.hh"

// XXX
#include <iostream>
#include <tuple>

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
}

Graph_V2::~Graph_V2() {
    delete [] group_index;
    delete [] idx2pos;
}

vector<Node_Id> Graph_V2::get_outgoing_edges(Node_Id node) {
    return get_edges(node, true, &Graph_V2::get_outgoing_edges_raw,
            &Graph_V2::get_incoming_edges_raw);
}

vector<Node_Id> Graph_V2::get_incoming_edges(Node_Id node) {
    return get_edges(node, false, &Graph_V2::get_incoming_edges_raw,
            &Graph_V2::get_outgoing_edges_raw);
}

size_t Graph_V2::get_node_count() {
    return group_index[group_index_length - 1];
}

/*
 * Old version that uses linear search.
 *
Group_Idx Graph_V2::get_group_index(Node_Id node) {
    size_t i;
    for (i = group_index_length - 2; i > 0; --i) {
        if (group_index[i] <= node) {
            break;
        }
    }
    return (Group_Idx) i;
}
*/

// XXX Assumes that 0 is in the array.
size_t bin_search(Node_Id a[], size_t length, Node_Id x) {
    size_t lo = 0;
    size_t hi = length - 1;
    while (lo <= hi) {
        size_t mid = lo + ((hi - lo) >> 1);
        Node_Id y = a[mid];
        if (x > y) {
            lo = mid + 1;
        } 
        else if (x < y) {
            hi = mid - 1;
        } else {
            return mid;
        }
    }
    assert(a[hi] < x && a[hi + 1] > x);
    return hi;
}

Graph_V2::Group_Idx Graph_V2::get_group_index(Node_Id node) {
    return (Group_Idx) bin_search(group_index, group_index_length - 1, node);
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

vector<Node_Id> Graph_V2::get_edges(Node_Id node, bool is_fwd,
        vector<Node_Id> (Graph_V2::*get_source_edges)(Group_Idx),
        vector<Node_Id> (Graph_V2::*get_dest_edges)(Group_Idx)) {
    Group_Idx group_idx = get_group_index(node);
    size_t sz = get_group_size(group_idx);
    vector<Node_Id> raw_edges = (this->*get_source_edges)(group_idx);
    if (sz < 2) {
        return raw_edges;
    }

    vector<Node_Id> edges;
    Node_Id my_lo = get_group_id(group_idx);
    Node_Id my_hi = my_lo + sz;

    if (is_fwd && node > 0 && node - 1 >= my_lo) {
        edges.push_back(node - 1);
    } else if (!is_fwd && node + 1 < my_hi) {
        edges.push_back(node + 1);
    }

    size_t my_idx = 0;
    size_t len = raw_edges.size();
    while (my_idx < len) {
        Group_Idx other_grp_idx = get_group_index(raw_edges[my_idx]);
        vector<Node_Id> other_edges = (this->*get_dest_edges)(other_grp_idx);
    
        for (Node_Id n : other_edges) {
            if (n < my_lo) {
                continue;
            }
            if (n >= my_hi) {
                break;
            }
            if (n == node) {
                edges.push_back(raw_edges[my_idx]);
            }
            ++my_idx;
        }
    }

    return edges;
}

Node_Id construct_edge_id(Node_Id src, Node_Id dest, int node_count) {
    size_t bits = nbits_for_int(node_count);
    return (dest << bits) + src + node_count;
}

ssize_t sorted_range_search(vector<Node_Id> v, Node_Id lo, Node_Id hi) {
    size_t sz = v.size();
    for (size_t i = 0; i < sz; ++i) {
        if (v[i] >= hi) {
            break;
        }
        if (v[i] >= lo) {
            return i;
        }
    }
    return -1;
}

vector<tuple<Node_Id, Node_Id>> match_edges(vector<Node_Id>::iterator& it,
        Node_Id my_lo, Node_Id my_hi, vector<Node_Id>& other) {
    ssize_t pos = sorted_range_search(other, my_lo, my_hi);
    if (pos < 0) {
        return {};
    }
    vector<tuple<Node_Id, Node_Id>> edges;
    for (vector<Node_Id>::iterator o = other.begin() + pos;
            o != other.end(); ++o) {
        if (*o >= my_hi) {
            return edges;
        }
        edges.push_back(make_tuple(*it, *o));
        ++it;
    }
    return edges;
}

map<string, vector<Node_Id>> Graph_V2::friends_of(Node_Id pathname, Node_Id task,
        Metadata* metadata) {
    /*
     * From pathname, get file inodes.
     * Get Group_Idx of task and each file_inode
     * Assert only one file inode
     * Find all edges (both forward and back)
     * Between file inode and graph
     * -> go from file because fewer edges
     * find a set of relations
     *
     * for all edges going out or in from task,
     * if relation matches
     * get group_idx of that side
     * if that group is a file inode
     * get path_name and add to output
     */

    string path = metadata->get_identifier(pathname);
    cout << metadata->get_metadata(path).at("cf:pathname") << endl;
    vector<Node_Id> pathname_edges = get_incoming_edges(pathname);
    assert(pathname_edges.size() == 1); 
    Group_Idx file_idx = get_group_index(pathname_edges[0]);
    Node_Id file_lo = get_group_id(file_idx);
    Node_Id file_hi = file_lo + get_group_size(file_idx);
    cout << file_lo << " " << file_hi << endl;
    string file_str = metadata->get_identifier(file_lo);
    cout << file_str << endl;
    cout << metadata->get_metadata(file_str).at("cf:type") << endl;

    Group_Idx task_idx = get_group_index(task);
    Node_Id task_lo = get_group_id(task_idx);
    Node_Id task_hi = task_lo + get_group_size(task_idx);

    cout << "task_lo " << task_lo << endl;
    cout << "task_hi " << task_hi << endl;

    vector<Node_Id> task_out = get_outgoing_edges_raw(task_idx);
    vector<Node_Id> task_in = get_incoming_edges_raw(task_idx);
    cout << "TASK_IN" << endl;
    for (Node_Id nid : task_in) {
        cout << nid << endl;
        string nid_str = metadata->get_identifier(nid);
        cout << nid_str << endl;
        cout << metadata->get_metadata(nid_str).at("cf:type") << endl;
    }
    cout << "END_TASK_IN" << endl;

    set<string> relations;
    vector<Node_Id> file_out = get_outgoing_edges_raw(file_idx);
    ssize_t pos = sorted_range_search(file_out, task_lo, task_hi);
    if (pos != -1) {
        cout << "searching outgoing" << endl;
        vector<Node_Id>::iterator fout = file_out.begin() + pos;
        auto edges = match_edges(fout, file_lo, file_hi, task_in);
        for (tuple<Node_Id, Node_Id> edge : edges) {
            Node_Id numeric_edge_id = construct_edge_id(get<1>(edge),
                    get<0>(edge), get_node_count());
            string edge_string = metadata->get_identifier(numeric_edge_id);
            string relation = metadata->get_metadata(edge_string).
                at("cf:type");
            relations.insert(relation);
            cout << "RELATION " << relation << endl;
        }
    }

    vector<Node_Id> file_in = get_incoming_edges_raw(file_idx);
    pos = sorted_range_search(file_in, task_lo, task_hi);
    if (pos != -1) {
        vector<Node_Id>::iterator fin = file_in.begin() + pos;
        auto edges = match_edges(fin, file_lo, file_hi, task_out);
        for (tuple<Node_Id, Node_Id> edge : edges) {
            Node_Id numeric_edge_id = construct_edge_id(get<0>(edge),
                    get<1>(edge), get_node_count());
            string edge_string = metadata->get_identifier(numeric_edge_id);
            string relation = metadata->get_metadata(edge_string).
                at("cf:type");
            relations.insert(relation);
        }
    }

    cout << "RELATIONS" << endl;
    for (auto relation : relations) {
        cout << relation << endl;
    }
    cout << "ENDRELATIONS" << endl;

    map<Group_Idx, set<string>> friends;
    vector<Node_Id>::iterator tout = task_out.begin();
    while (tout != task_out.end()) {
        Node_Id n = *tout;
        string nid = metadata->get_identifier(n);
        Group_Idx nidx = get_group_index(n);
        if (nidx == file_idx || metadata->get_metadata(nid).
                at("cf:type") != "file") {
            Node_Id nhi = get_group_id(nidx) + get_group_size(nidx);
            for (; tout != task_out.end() && *tout < nhi; ++tout) {
                // skip ahead
            }
            continue;
        }

        vector<Node_Id> other = get_incoming_edges_raw(nidx);
        auto edges = match_edges(tout, task_lo, task_hi, other);
        for (tuple<Node_Id, Node_Id> edge : edges) {
            Node_Id friendly = get<0>(edge);
            Node_Id numeric_edge_id = construct_edge_id(get<1>(edge),
                    friendly, get_node_count());
            string edge_string = metadata->get_identifier(numeric_edge_id);
            string relation = metadata->get_metadata(edge_string).
                at("cf:type");
            cout << "relation " << relation << endl;
            if (!relations.count(relation)) {
                cout << "NOPE" << endl;
                continue;
            }
            string friend_id = metadata->get_identifier(friendly);
            if (metadata->get_metadata(friend_id).at("cf:type") == "file") {
                Group_Idx friend_idx = get_group_index(friendly);
                if (!friends.count(friend_idx)) {
                    friends[friend_idx] = {};
                }
                friends[friend_idx].insert(relation);
            }
        }
    }

    for (Node_Id id : task_in) {
        cout << id << endl;
        Group_Idx idx = get_group_index(id);
        cout << idx << endl;
        string nid = metadata->get_identifier(id);
        cout << nid << endl;
        cout << metadata->get_metadata(nid).at("cf:type") << endl;
    }
    cout << "my id" << get_group_id(file_idx) << endl; 
    vector<Node_Id>::iterator tin = task_in.begin();
    while (tin != task_in.end()) {
        Node_Id n = *tin;
        string nid = metadata->get_identifier(n);
        Group_Idx nidx = get_group_index(n);
        cout << n << endl;
        cout << metadata->get_metadata(nid).at("cf:type") << endl;
        if (nidx == file_idx || metadata->get_metadata(nid).
                at("cf:type") != "file") {
            Node_Id nhi = get_group_id(nidx) + get_group_size(nidx);
            for (; tin != task_in.end() && *tin < nhi; ++tin) {
                // skip ahead
            }
            continue;
        }
        cout << "made it!" << endl;

        vector<Node_Id> other = get_outgoing_edges_raw(nidx);
        auto edges = match_edges(tin, task_lo, task_hi, other);
        for (tuple<Node_Id, Node_Id> edge : edges) {
            cout << "found something" << endl;
            Node_Id friendly = get<1>(edge);
            Node_Id numeric_edge_id = construct_edge_id(get<0>(edge),
                    friendly, get_node_count());
            string edge_string = metadata->get_identifier(numeric_edge_id);
            string relation = metadata->get_metadata(edge_string).
                at("cf:type");
            cout << "relation " << relation << endl;
            if (!relations.count(relation)) {
                continue;
            }
            string friend_id = metadata->get_identifier(friendly);
            if (metadata->get_metadata(friend_id).at("cf:type") == "file") {
                Group_Idx friend_idx = get_group_index(friendly);
                if (!friends.count(friend_idx)) {
                    friends[friend_idx] = {};
                }
                friends[friend_idx].insert(relation);
            }
        }
    }

    map<string, vector<Node_Id>> output;
    for (auto p : friends) {
        Node_Id n = get_group_id(p.first);
        vector<Node_Id> out = get_outgoing_edges(n);
        assert(out.size() == 1);
        for (string rel : p.second) {
            if (!output.count(rel)) {
                output[rel] = {};
            }
            output[rel].push_back(out[0]);
        }
    }

    for (auto p : output) {
        cout << p.first << endl;
        for (Node_Id id : p.second) {
            cout << metadata->get_identifier(id) << endl;
        }
    }

    return output;
}
