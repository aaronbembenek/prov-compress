#include <queue>
#include <set>

#include "graph.hh"

using namespace std;

// XXX Currently untested...
vector<NodeId> Graph::getAllDescendants(NodeId node) {
    vector<NodeId> neighbors = getOutgoingEdges(node);
    queue<NodeId> q;
    set<NodeId> visited;
    for (NodeId neighbor : neighbors) {
        q.push(neighbor);
        visited.insert(neighbor);
    }
    while (!q.empty()) {
        NodeId nid = q.front();
        q.pop();
        for (NodeId nid2 : getOutgoingEdges(nid)) {
            if (!visited.count(nid2)) {
                visited.insert(nid2);
                q.push(nid2);
            }
        }
    }
    return vector<NodeId>(visited.begin(), visited.end());
}
