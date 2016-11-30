#include "graph_v1.hh"
#include "graph_dummy.hh"
#include "queriers.hh"

DummyQuerier::DummyQuerier(string& auditfile) {
    metadata_ = new DummyMetadata(auditfile);
    graph_ = new DummyGraph(metadata_);
}

CompressedQuerier::CompressedQuerier(string& metafile, string& graphfile) {
    metadata_ = new CompressedMetadata(metafile);
    string buffer;
    read_file(graphfile, buffer);
    graph_ = new Graph_V1(buffer);
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
void Querier::friends_of(string& identifier) {
    (void)identifier;
}
vector<string> Querier::get_ids() {
    return metadata_->get_ids();
}


