#include "graph_v1.hh"
#include "queriers.hh"

/*
 * Querier on Uncompressed Data
 */
DummyQuerier::DummyQuerier(string& auditfile) {
    metadata_ = new DummyMetadata(auditfile);
}
map<string, string> DummyQuerier::get_metadata(string& identifier) {
    return metadata_->get_metadata(identifier);
}
vector<Node_Id> DummyQuerier::get_all_ancestors(string& identifier) {
    (void)identifier;
    return {};
}
vector<Node_Id> DummyQuerier::get_direct_ancestors(string& identifier) {
    (void)identifier;
    return {};
}
vector<Node_Id> DummyQuerier::get_all_descendants(string& identifier) {
    (void)identifier;
    return {};
}
vector<Node_Id> DummyQuerier::get_direct_descendants(string& identifier) {
    (void)identifier;
    return {};
}
vector<vector<Node_Id>> DummyQuerier::all_paths(string& sourceid, string& sinkid) {
    (void)sourceid;
    (void)sinkid;
    return {};
}
void DummyQuerier::friends_of(string& identifier) {
    (void)identifier;
}
vector<string> DummyQuerier::get_ids() {
    return metadata_->get_ids();
}

/*
 * Querier on Compressed Graph and Metadata
 */
CompressedQuerier::CompressedQuerier(string& metafile, string& graphfile) {
    metadata_ = new CompressedMetadata(metafile);
    string buffer;
    read_file(graphfile, buffer);
    graph = new Graph_V1(buffer);
}
map<string, string> CompressedQuerier::get_metadata(string& identifier) {
    return metadata_->get_metadata(identifier);
}
vector<Node_Id> CompressedQuerier::get_all_ancestors(string& identifier) {
    Node_Id node = metadata_->get_node_id(identifier);
    return graph->get_all_ancestors(node);
}
vector<Node_Id> CompressedQuerier::get_direct_ancestors(string& identifier) {
    Node_Id node = metadata_->get_node_id(identifier);
    return graph->get_incoming_edges(node);
}
vector<Node_Id> CompressedQuerier::get_all_descendants(string& identifier) {
    Node_Id node = metadata_->get_node_id(identifier);
    return graph->get_all_descendants(node);
}
vector<Node_Id> CompressedQuerier::get_direct_descendants(string& identifier) {
    Node_Id node = metadata_->get_node_id(identifier);
    return graph->get_outgoing_edges(node);
}
vector<vector<Node_Id>> CompressedQuerier::all_paths(string& sourceid, string& sinkid) {
    Node_Id source = metadata_->get_node_id(sourceid);
    Node_Id sink = metadata_->get_node_id(sinkid);
    return graph->get_all_paths(source, sink);
}
void CompressedQuerier::friends_of(string& identifier) {
    (void)identifier;
}
vector<string> CompressedQuerier::get_ids() {
    return metadata_->get_ids();
}

