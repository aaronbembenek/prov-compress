#ifndef GRAPHV1_HH
#define GRAPHV1_HH

#include "graph.hh"
#include "helpers.hh"

#include <string>

class GraphV1 : public Graph {
    public:
        GraphV1(std::string&);
        ~GraphV1();
        std::vector<NodeId> getOutgoingEdges(NodeId);
        size_t getNodeCount();
    private:
        BitSet data;
        size_t* index;
        size_t indexLength;
        size_t nbitsForDegree;
        size_t nbitsForDelta;
        size_t basePos;
};

#endif
