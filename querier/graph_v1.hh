#ifndef GRAPH_V1_HH
#define GRAPH_V1_HH

#include "graph.hh"
#include "helpers.hh"

#include <string>

class Graph_V1 : public Graph {
    public:
        Graph_V1(std::string&);
        ~Graph_V1();
        std::vector<Node_Id> get_outgoing_edges(Node_Id) override;
        size_t get_node_count() override;
    private:
        BitSet data;
        size_t* index;
        size_t index_length;
        size_t nbits_degree;
        size_t nbits_delta;
        size_t base_pos;
};

#endif
