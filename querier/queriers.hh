#ifndef QUERY_H
#define QUERY_H

#include "helpers.hh"
#include "metadata.hh"
#include "graph_dummy.hh"

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
    virtual map<string, string> get_metadata(string& identifier) = 0;
    virtual vector<Node_Id> get_all_ancestors(string& identifier) = 0;
    virtual vector<Node_Id> get_direct_ancestors(string& identifier) = 0;
    virtual vector<Node_Id> get_all_descendants(string& identifier) = 0;
    virtual vector<Node_Id> get_direct_descendants(string& identifier) = 0;
    virtual vector<vector<Node_Id>> all_paths(string& sourceid, string& sinkid) = 0;
    virtual void friends_of(string& identifier) = 0;
    virtual vector<string> get_ids() = 0;
};

class DummyQuerier : Querier {
public:
    DummyQuerier(string& auditfile);
    map<string, string> get_metadata(string& identifier) override;
    vector<Node_Id> get_all_ancestors(string& identifier) override;
    vector<Node_Id> get_direct_ancestors(string& identifier) override;
    vector<Node_Id> get_all_descendants(string& identifier) override;
    vector<Node_Id> get_direct_descendants(string& identifier) override;
    vector<vector<Node_Id>> all_paths(string& sourceid, string& sinkid) override;
    void friends_of(string& identifier) override;
    vector<string> get_ids() override;
    
private:
    DummyMetadata* metadata_;
    Graph_Dummy* graph_;
};

class CompressedQuerier: Querier {
public:
    CompressedQuerier(string& metafile, string& graphfile);
    map<string, string> get_metadata(string& identifier) override;
    vector<Node_Id> get_all_ancestors(string& identifier) override;
    vector<Node_Id> get_direct_ancestors(string& identifier) override;
    vector<Node_Id> get_all_descendants(string& identifier) override;
    vector<Node_Id> get_direct_descendants(string& identifier) override;
    vector<vector<Node_Id>> all_paths(string& sourceid, string& sinkid) override;
    void friends_of(string& identifier) override;
    vector<string> get_ids() override;
    
private:
    CompressedMetadata* metadata_;
    Graph* graph;
};

#endif /* QUERY_H */
