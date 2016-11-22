#ifndef GRAPH_HH
#define GRAPH_HH

#include <cstddef>
#include <vector>

typedef size_t NodeId;

class Graph {
    public:
        virtual std::vector<NodeId> getOutgoingEdges(NodeId) = 0;
        virtual size_t getNodeCount() = 0;
        std::vector<NodeId> getAllDescendants(NodeId);
};

#endif
