#include <map>
#include <queue>
#include <set>

#include "graph.hh"

using namespace std;

vector<Node_Id> Graph::get_all_descendants(Node_Id node) {
    return bfs_helper(node, &Graph::get_outgoing_edges);
}

vector<Node_Id> Graph::get_all_ancestors(Node_Id node) {
    return bfs_helper(node, &Graph::get_incoming_edges);
}

vector<Node_Id> Graph::bfs_helper(Node_Id node,
        vector<Node_Id> (Graph::*get_neighbors)(Node_Id)) {
    vector<Node_Id> neighbors = (this->*get_neighbors)(node);
    queue<Node_Id> q;
    set<Node_Id> visited;
    for (Node_Id neighbor : neighbors) {
        q.push(neighbor);
        visited.insert(neighbor);
    }
    while (!q.empty()) {
        Node_Id nid = q.front();
        q.pop();
        for (Node_Id nid2 : (this->*get_neighbors)(nid)) {
            if (!visited.count(nid2)) {
                visited.insert(nid2);
                q.push(nid2);
            }
        }
    }
    return vector<Node_Id>(visited.begin(), visited.end());
}

vector<vector<Node_Id>> Graph::get_all_paths(Node_Id source, Node_Id sink) {
    typedef vector<Node_Id> v;
    struct Entry {
        Node_Id node;
        v::iterator it;
        vector<v> acc;
    };

    map<Node_Id, v> memo;
    memo[source] = get_outgoing_edges(source);
    vector<Entry> stack {{source, memo[source].begin(), {}}};
    vector<v> acc;
    bool forward = true;
    while (!stack.empty()) {
        Entry& top = stack.back();

        if (!forward) {
            for (v path : acc) {
                path.push_back(top.node);
                top.acc.push_back(path);
            }
        }

        if (top.node == sink) {
            top.acc.push_back({sink});
        } else {
            if (top.it != memo[top.node].end()) {
                Node_Id child = *(top.it++);
                if (memo.find(child) == memo.end()) {
                    memo[child] = get_outgoing_edges(child);
                }
                stack.push_back({child, memo[child].begin(), {}});
                forward = true;
                continue;
            }
        }

        forward = false;
        acc = top.acc;
        stack.pop_back();
    }

    vector<v> r;
    for (v path : acc) {
        r.push_back(v(path.rbegin(), path.rend()));
    }
    return r;
}

/*
vector<vector<Node_Id>> Graph::get_all_paths(Node_Id source, Node_Id sink) {
    vector<vector<Node_Id>> a;
    vector<vector<Node_Id>*> paths = all_paths_helper(source, sink);
    while (!paths.empty()) {
        vector<Node_Id>* path = paths.back();
        paths.pop_back();
        a.push_back(vector<Node_Id>(path->rbegin(), path->rend()));
        delete path;
    }
    return a;
}

vector<vector<Node_Id>*> Graph::all_paths_helper(Node_Id cur, Node_Id dest) {
    if (cur == dest) {
        vector<Node_Id>* v = new vector<Node_Id>({cur});
        return vector<vector<Node_Id>*>({v});
    }
    vector<vector<Node_Id>*> a = vector<vector<Node_Id>*>();
    for (Node_Id child : get_outgoing_edges(cur)) {
        vector<vector<Node_Id>*> paths = all_paths_helper(child, dest);
        for (vector<Node_Id>* path : paths) {
            path->push_back(cur);
            a.push_back(path);
        }
    }
   return a;
}
*/
