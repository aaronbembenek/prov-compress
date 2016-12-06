#include "graph_v1.hh"
#include "json_graph.hh"
#include "queriers.hh"

DummyQuerier::DummyQuerier(string& auditfile) {
    auto json_graph = new JsonGraph(auditfile);
    metadata_ = json_graph; 
    graph_ = json_graph;
}

CompressedQuerier::CompressedQuerier(string& metafile, string& graphfile) {
    metadata_ = new CompressedMetadata(metafile);
    string buffer;
    read_file(graphfile, buffer);
    graph_ = new Graph_V1(buffer);
}

map<string, vector<string>> Querier::friends_of(string& file_id, string& task_id) {
    Node_Id file_node = metadata_->get_node_id(file_id);
    Node_Id task_node = metadata_->get_node_id(task_id);
    auto relation2nodeids = graph_->friends_of(file_node, task_node, metadata_);


    map<string, vector<string>> relation2ids;
    for (auto pair : relation2nodeids) {
        for (auto n : pair.second) {
            relation2ids[pair.first].push_back(metadata_->get_identifier(n));
        }
    }
    return relation2ids;
}
map<string, string> Querier::get_metadata(string& identifier) {
    return metadata_->get_metadata(identifier);
}
vector<string> Querier::get_all_ancestors(string& identifier) {
    Node_Id node = metadata_->get_node_id(identifier);
    auto node_ids = graph_->get_all_ancestors(node);

    vector<string> ids;
    for (auto n : node_ids) {
        ids.push_back(metadata_->get_identifier(n));
    }
    return ids;
}
vector<string> Querier::get_direct_ancestors(string& identifier) {
    Node_Id node = metadata_->get_node_id(identifier);
    auto node_ids = graph_->get_incoming_edges(node);

    vector<string> ids;
    for (auto n : node_ids) {
        ids.push_back(metadata_->get_identifier(n));
    }
    return ids;
}
vector<string> Querier::get_all_descendants(string& identifier) {
    Node_Id node = metadata_->get_node_id(identifier);
    auto node_ids = graph_->get_all_descendants(node);

    vector<string> ids;
    for (auto n : node_ids) {
        ids.push_back(metadata_->get_identifier(n));
    }
    return ids;
}
vector<string> Querier::get_direct_descendants(string& identifier) {
    Node_Id node = metadata_->get_node_id(identifier);
    auto node_ids = graph_->get_outgoing_edges(node);
    
    vector<string> ids;
    for (auto n : node_ids) {
        ids.push_back(metadata_->get_identifier(n));
    }
    return ids;
}
vector<vector<string>> Querier::all_paths(string& sourceid, string& sinkid) {
    Node_Id source = metadata_->get_node_id(sourceid);
    Node_Id sink = metadata_->get_node_id(sinkid);
    vector<vector<Node_Id>> node_id_paths = graph_->get_all_paths(source, sink);
    
    vector<vector<string>> result;
    for (auto node_ids  : node_id_paths) {
        vector<string> ids;
        for (auto n : node_ids) {
            ids.push_back(metadata_->get_identifier(n));
        }
        result.push_back(ids);
    }
    return result;
}
vector<string> Querier::get_node_ids() {
    return metadata_->get_node_ids();
}
