#ifndef GRAPH_V1_HH
#define GRAPH_V1_HH

#include "graph.hh"
#include "helpers.hh"

class Graph_V1 : public Graph {
    public:
        Graph_V1(std::string&);
        ~Graph_V1();
        std::vector<Node_Id> get_outgoing_edges(Node_Id) override;
        std::vector<Node_Id> get_incoming_edges(Node_Id) override;
        size_t get_node_count() override;
    private:
        BitSet data;
        size_t* index;
        size_t index_length;
        size_t nbits_outdegree;
        size_t nbits_outdelta;
        size_t nbits_indegree;
        size_t nbits_indelta;
        size_t base_pos;

        std::vector<Node_Id> get_edges(Node_Id, size_t, size_t, size_t);
};

#endif
