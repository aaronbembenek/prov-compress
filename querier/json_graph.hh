#ifndef JSON_GRAPH_HH
#define JSON_GRAPH_HH

#include "metadata.hh"
#include "graph.hh"

class JsonGraph: public Metadata, public Graph {
    static vector<string> typs;
public:
	map<Node_Id, vector<Node_Id>> graph_;
	map<Node_Id, vector<Node_Id>> tgraph_;
private:
    map<string, string> id2jsonstr;
    map<string, Node_Id>id2nodeid;
    map<Node_Id, string>nodeid2id;
    vector<string> node_ids;
    vector<string> relation_ids;

    void construct_graph();
public:
    JsonGraph(string& infile);
    map<string, string> get_metadata(string& identifier) override;
    Node_Id get_node_id(string) override;
    string get_identifier(Node_Id) override;
    vector<string> get_node_ids() override;

    std::vector<Node_Id> get_outgoing_edges(Node_Id);
    std::vector<Node_Id> get_incoming_edges(Node_Id);
    size_t get_node_count();
};

#endif
