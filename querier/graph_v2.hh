#ifndef GRAPH_V2_HH
#define GRAPH_V2_HH

#include "graph.hh"
#include "helpers.hh"

#if BESAFE
#include <boost/serialization/strong_typedef.hpp>
#endif

class Graph_V2 : public Graph {
    public:
        Graph_V2(std::string&);
        ~Graph_V2();
        std::vector<Node_Id> get_outgoing_edges(Node_Id) override;
        std::vector<Node_Id> get_incoming_edges(Node_Id) override;
        std::map<std::string, std::vector<Node_Id>> friends_of(Node_Id, Node_Id)
            override;
        size_t get_node_count() override;
    private:
#if BESAFE
        BOOST_STRONG_TYPEDEF(size_t, Group_Idx)
#else
        typedef size_t Group_Idx;
#endif
        struct info_t {
            size_t nbits_degree;
            size_t nbits_delta;
        };
        BitSet data;
        Node_Id* group_index;
        size_t group_index_length;
        size_t base_pos;
        size_t* idx2pos;
        map<bool, info_t> fwd_info;
        map<bool, info_t> back_info;

        Group_Idx get_group_index(Node_Id);
        size_t get_group_size(Group_Idx);
        Node_Id get_group_id(Group_Idx);
        size_t get_group_pos(Group_Idx);

        std::vector<Node_Id> read_edges_raw(Group_Idx, size_t, info_t);
        std::vector<Node_Id> get_outgoing_edges_raw(Group_Idx);
        std::vector<Node_Id> get_incoming_edges_raw(Group_Idx);
        std::vector<Node_Id> get_edges(Node_Id, bool,
                std::vector<Node_Id> (Graph_V2::*)(Group_Idx),
                std::vector<Node_Id> (Graph_V2::*)(Group_Idx));
};

#endif
