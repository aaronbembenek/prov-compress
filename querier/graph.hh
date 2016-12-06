#ifndef GRAPH_HH
#define GRAPH_HH

#include <cstddef>
#include <vector>
#include <map>
#include <string>

typedef size_t Node_Id;

class Graph {
    public:
        virtual std::vector<Node_Id> get_outgoing_edges(Node_Id) = 0;
        virtual std::vector<Node_Id> get_incoming_edges(Node_Id) = 0;
        virtual std::map<std::string, std::vector<Node_Id>> friends_of(Node_Id, Node_Id) = 0;
        virtual size_t get_node_count() = 0;
        std::vector<Node_Id> get_all_descendants(Node_Id);
        std::vector<Node_Id> get_all_ancestors(Node_Id);
        std::vector<std::vector<Node_Id>> get_all_paths(Node_Id, Node_Id);

    private:
        std::vector<Node_Id> bfs_helper(Node_Id,
                std::vector<Node_Id> (Graph::*)(Node_Id));
        /*
        std::vector<std::vector<Node_Id>*> all_paths_helper(Node_Id, Node_Id);
        */
};

#endif
