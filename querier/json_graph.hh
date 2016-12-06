#ifndef JSON_GRAPH_HH
#define JSON_GRAPH_HH

#include "metadata.hh"
#include "graph.hh"

class JsonGraph: public Metadata, public Graph {
    static vector<string> typs;
    typedef int Task_Id;
    typedef int File_Id;
public:
	map<Node_Id, vector<Node_Id>> graph_;
	map<Node_Id, vector<Node_Id>> tgraph_;
private:
    map<string, string> id2jsonstr;
    map<string, Node_Id>id2nodeid;
    map<Node_Id, string>nodeid2id;
    vector<string> node_ids;
    vector<string> relation_ids;

    map<File_Id, map<string, set<Task_Id>>> file2tasks;
    map<Task_Id, map<string, set<File_Id>>> task2files;
    map<Node_Id, File_Id> pathname2file;
    map<File_Id, Node_Id> file2pathname;

    void construct_graph();
public:
    JsonGraph(string& infile);
    map<string, string> get_metadata(string& identifier) override;
    Node_Id get_node_id(string) override;
    string get_identifier(Node_Id) override;
    vector<string> get_node_ids() override;
    
    vector<Node_Id> friends_of(Node_Id, Node_Id) override;
    std::vector<Node_Id> get_outgoing_edges(Node_Id) override;
    std::vector<Node_Id> get_incoming_edges(Node_Id) override;
    size_t get_node_count() override;
};

#endif
