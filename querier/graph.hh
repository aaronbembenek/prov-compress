#ifndef GRAPH_HH
#define GRAPH_HH

#include <vector>

typedef unsigned int NodeId;

class Graph {
    public:
        virtual std::vector<NodeId> getOutgoingEdges(NodeId) = 0;
        std::vector<NodeId> getAllDescendants(NodeId);
};

#endif
