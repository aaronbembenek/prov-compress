#ifndef GRAPH_Dummy_HH
#define GRAPH_Dummy_HH

#include "graph.hh"
#include "metadata.hh"
#include "helpers.hh"

#include <string>

class Graph_Dummy : public Graph {
    public:
        Graph_Dummy(DummyMetadata* metadata);
        ~Graph_Dummy();
        std::vector<Node_Id> get_outgoing_edges(Node_Id) override;
        std::vector<Node_Id> get_incoming_edges(Node_Id) override;
        size_t get_node_count() override;
    private:
        DummyMetadata* metadata_;
        map<string, vector<string>> graph_;
        map<string, vector<string>> tgraph_;
};

#endif
