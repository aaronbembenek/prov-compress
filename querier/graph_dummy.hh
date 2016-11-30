#ifndef GRAPH_Dummy_HH
#define GRAPH_Dummy_HH

#include "graph.hh"
#include "metadata.hh"
#include "helpers.hh"

#include <string>

class DummyGraph : public Graph {
    public:
        DummyGraph(Metadata* metadata);
        ~DummyGraph();
        std::vector<Node_Id> get_outgoing_edges(Node_Id) override;
        std::vector<Node_Id> get_incoming_edges(Node_Id) override;
        size_t get_node_count() override;
    private:
        Metadata* metadata_;
        map<Node_Id, vector<Node_Id>> graph_;
        map<Node_Id, vector<Node_Id>> tgraph_;
};

#endif
