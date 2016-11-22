#include "graphv1.hh"
#include "helpers.hh"

#include <iostream>

GraphV1::GraphV1(string& compressed) : data(compressed) {
    // First 32 bits have basic metadata.
    int pos = 0;
    pos += data.get_bits<size_t>(nbitsForDegree, 8, pos);
    pos += data.get_bits<size_t>(nbitsForDelta, 8, pos);
    size_t nbitsForIndexEntry;
    pos += data.get_bits<size_t>(nbitsForIndexEntry, 8, pos);
    pos += data.get_bits<size_t>(indexLength, 8, pos);
    indexLength += 1;

    // Create index from node ID into node data.
    index = new size_t[indexLength];
    index[0] = 0;
    for (size_t i = 1; i < indexLength; ++i) {
        pos += data.get_bits<size_t>(index[i], nbitsForIndexEntry, pos);
        index[i] += index[i - 1];
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
    for (size_t i = 1; i < outdegree; ++i) {
        NodeId child;
        pos += data.get_bits<NodeId>(child, nbitsForDelta, pos);
        children.push_back(children.back() + child);
    }
    return children;
}

size_t GraphV1::getNodeCount() {
    return indexLength;
}

GraphV1::~GraphV1() {
    delete [] this->index;
}

int main(int argc, char* argv[]) {
    string buffer;
    if (argc < 2) {
        read_file("samples/tiny.cpg", buffer);
    } else {
        read_file(argv[1], buffer);
    }
    GraphV1 graph = GraphV1(buffer);
    size_t n = graph.getNodeCount();
    for (NodeId i = 0; i < n; ++i) {
        cout << "Node " << i << "'s children:" << endl;
        for (auto child : graph.getOutgoingEdges(i)) {
            cout << child << endl;
        }
    }
    for (NodeId i = 0; i < n; ++i) {
        cout << "Descendants of " << i << ":" << endl;
        for (auto descendant : graph.getAllDescendants(i)) {
            cout << descendant << endl;
        }
    }
}
