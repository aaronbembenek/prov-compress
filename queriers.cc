#include "queriers.hh"

CompressedQuerier::CompressedQuerier(string& metafile, string& graphfile) {
    (void)(graphfile);
    metadata_ = new Metadata(metafile);
}

map<string, string> CompressedQuerier::get_metadata(string& identifier) {
    return metadata_->get_metadata(identifier);
}
// TODO change signatures to return graph
void CompressedQuerier::get_all_ancestors(string& identifier) {
    (void)identifier;
}
void CompressedQuerier::get_direct_ancestors(string& identifier) {
    (void)identifier;
}
void CompressedQuerier::get_all_descendants(string& identifier) {
    (void)identifier;
}
void CompressedQuerier::get_direct_descendants(string& identifier) {
    (void)identifier;
}
void CompressedQuerier::all_paths(string& sourceid, string& sinkid) {
    (void)sourceid;
    (void)sinkid;
}
void CompressedQuerier::friends_of(string& identifier) {
    (void)identifier;
}
