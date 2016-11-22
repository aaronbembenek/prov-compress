#ifndef GRAPH_HH
#define GRAPH_HH

#include <cstddef>
#include <vector>

typedef size_t Node_Id;

class Graph {
    public:
        virtual std::vector<Node_Id> get_outgoing_edges(Node_Id) = 0;
        virtual size_t get_node_count() = 0;
        std::vector<Node_Id> get_all_descendants(Node_Id);
};

#endif
