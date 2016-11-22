#include "graphv1.hh"
#include "helpers.hh"

#include <iostream>

GraphV1::GraphV1(string& compressed) : data(compressed) {
    // First 32 bits have basic metadata.
    int pos = 0;
    pos += data.get_bits<size_t>(nbitsForDegree, 8, pos);
    pos += data.get_bits<size_t>(nbitsForDelta, 8, pos);
    size_t nbitsForIndexEntry, sizeofIndex;
    pos += data.get_bits<size_t>(nbitsForIndexEntry, 8, pos);
    pos += data.get_bits<size_t>(indexLength, 8, pos);

    // Create index from node ID into node data.
    index = new size_t[indexLength];
    for (int i = 0; i < indexLength; ++i) {
        data.get_bits<size_t>(index[i], nbitsForIndexEntry, pos);
        pos += nbitsForIndexEntry;
    }
    basePos = ((pos + 7) >> 3) << 3;

    /*
    cout << "nbitsForDegree: " << nbitsForDegree << endl;
    cout << "nbitsForDelta: " << nbitsForDelta << endl;
    cout << "nbitsForIndexEntry: " << nbitsForIndexEntry << endl;
    cout << "indexLength: " << indexLength << endl;
    for (int i = 0; i < indexLength; ++i) {
        cout << index[i] << endl;
    }
    */
}

vector<NodeId> GraphV1::getOutgoingEdges(NodeId node) {
    assert(node < indexLength);
    size_t pos = index[node] + basePos;
    size_t outdegree;
    pos += data.get_bits<size_t>(outdegree, nbitsForDegree, pos);
    if (!outdegree) {
        return {};
    }
    vector<NodeId> children;
    NodeId first;
    pos += data.get_bits<NodeId>(first, nbitsForDelta + 1, pos);
    if (first % 2) {
        first = node - (first - 1) / 2;
    } else {
        first = node + first / 2;
    }
    children.push_back(first);
    for (int i = 1; i < outdegree; ++i) {
        NodeId child;
        pos += data.get_bits<NodeId>(child, nbitsForDelta, pos);
        children.push_back(children.back() + child);
    }
    return children;
}

GraphV1::~GraphV1() {
    delete [] this->index;
}

/*
int main(int argc, char* argv[]) {
    string buffer;
    read_file(argv[1], buffer);
    GraphV1 graph = GraphV1(buffer);
    for (NodeId i = 0; i < 8; ++i) {
        cout << "Node " << i << endl;
        for (auto child : graph.getOutgoingEdges(i)) {
            cout << child << endl;
        }
    }
    for (NodeId i = 0; i < 8; ++i) {
        cout << "Descendants of " << i << endl;
        for (auto descendant : graph.getAllDescendants(i)) {
            cout << descendant << endl;
        }
    }
}
*/
