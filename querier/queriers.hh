#ifndef QUERY_H
#define QUERY_H

#include "helpers.hh"
#include "metadata.hh"
#include "graph.hh"

/*
    SUPPORTED QUERIES:
    direct_ancestor(identifier) => return identifier
    all_ancestors(identifier) => return list of identifiers
    all_descendants(identifier) => return list of identifiers
    all_paths(source, sink) => return list of list of identifiers
    friends(identifier) => return list of identifiers (is this a useful query to support?)
    metadata(identifier) => return (JSON output?) of identifier
 */

class Querier {
public:
    Querier() {};
    map<string, string> get_metadata(string& identifier);
    vector<string> get_all_ancestors(string& identifier);
    vector<string> get_direct_ancestors(string& identifier);
    vector<string> get_all_descendants(string& identifier);
    vector<string> get_direct_descendants(string& identifier);
    vector<vector<string>> all_paths(string& sourceid, string& sinkid);
    virtual vector<string> friends_of(string&, string&) = 0;
    vector<string> get_node_ids();
    
protected:
    Metadata* metadata_;
    Graph* graph_;
};

class DummyQuerier : public Querier {
public:
    DummyQuerier(string& auditfile);
    vector<string> friends_of(string&, string&) override;
};

class CompressedQuerier: public Querier {
public:
    CompressedQuerier(string& metafile, string& graphfile);
    vector<string> friends_of(string&, string&) override;
};

#endif /* QUERY_H */
