#ifndef GRAPH_V2_HH
#define GRAPH_V2_HH

#include "graph.hh"
#include "helpers.hh"

#ifdef BESAFE
#include <boost/serialization/strong_typedef.hpp>
BOOST_STRONG_TYPEDEF(size_t, group_t)
#else
typedef size_t group_t;
#endif

class Graph_V2 : public Graph {
    public:
        Graph_V2(std::string&);
        ~Graph_V2();
        std::vector<Node_Id> get_outgoing_edges(Node_Id) override;
        std::vector<Node_Id> get_incoming_edges(Node_Id) override;
        size_t get_node_count() override;
    private:
        struct info {
            size_t nbits_degree;
            size_t nbits_delta;
        };
        BitSet data;
        Node_Id* group_index;
        size_t group_index_length;
        size_t base_pos;
        map<Node_Id, size_t> id2pos;
        map<bool, info> fwd_info;
        map<bool, info> back_info;

        group_t get_group(Node_Id);
        size_t get_group_size(group_t);
        Node_Id get_group_id(group_t);

        std::vector<Node_Id> read_edges_raw(group_t, size_t, info);
        std::vector<Node_Id> get_outgoing_edges_raw(group_t);
        std::vector<Node_Id> get_incoming_edges_raw(group_t);
        std::vector<Node_Id> get_edges(Node_Id, bool,
                std::vector<Node_Id> (Graph_V2::*)(group_t),
                std::vector<Node_Id> (Graph_V2::*)(group_t));
};

#endif
