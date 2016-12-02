#include "graph.hh"
#include "graph_v1.hh"
#include "graph_dummy.hh"
#include "metadata.hh"
#include "helpers.hh"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    string buffer;
    if (argc < 2) {
        read_file("../compression/graph.cpg", buffer);
    } else {
        cout << "Reading from " << argv[1] << endl;
        read_file(argv[1], buffer);
    }
    Graph* graph = new Graph_V1(buffer);
    string infile = "../compression/compressed_metadata.txt";
    Metadata* metadata = new CompressedMetadata(infile);
    size_t n = graph->get_node_count();
    for (Node_Id i = 0; i < n; ++i) {
        cout << "NODE " << metadata->get_identifier(i) << endl;
        cout << "Children:" << endl;
        for (auto child : graph->get_outgoing_edges(i)) {
            cout << metadata->get_identifier(child) << endl;
        }
        cout << "Descendants:" << endl;
        for (auto descendant : graph->get_all_descendants(i)) {
            cout << metadata->get_identifier(descendant) << endl;
        }
        cout << "Parents:" << endl;
        for (auto child : graph->get_incoming_edges(i)) {
            cout << metadata->get_identifier(child) << endl;
        }
        cout << "Ancestors:" << endl;
        for (auto child : graph->get_all_ancestors(i)) {
            cout << metadata->get_identifier(child) << endl;
        }
        cout << endl;
    }

    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            cout << "ALL PATHS BETWEEN " << i << " and " << j << endl;
            auto paths = graph->get_all_paths(i, j);
            for (auto path : paths) {
                for (auto node : path) {
                    cout << metadata->get_identifier(node) << endl;
                }
                cout << "----------" << endl;
            }
        }
    }

    string auditfile = "../hello_audit.log";
    metadata = new DummyMetadata(auditfile);
    graph = new DummyGraph(metadata);
/*
    for (auto id : metadata->get_ids(graph->get_node_count())) {
        Node_Id i = metadata->get_node_id(id);
        cout << "NODE " << id << endl;
        cout << "Children:" << endl;
        for (auto child : graph->get_outgoing_edges(i)) {
            cout << metadata->get_identifier(child) << endl;
        }
        cout << "Descendants:" << endl;
        for (auto descendant : graph->get_all_descendants(i)) {
            cout << metadata->get_identifier(descendant) << endl;
        }
        cout << "Parents:" << endl;
        for (auto child : graph->get_incoming_edges(i)) {
            cout << metadata->get_identifier(child) << endl;
        }
        cout << "Ancestors:" << endl;
        for (auto child : graph->get_all_ancestors(i)) {
            cout << metadata->get_identifier(child) << endl;
        }
        cout << endl;
    }

    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            cout << "ALL PATHS BETWEEN " << i << " and " << j << endl;
            auto paths = graph->get_all_paths(i, j);
            for (auto path : paths) {
                for (auto node : path) {
                    cout << metadata->get_identifier(node) << endl;
                }
                cout << "----------" << endl;
            }
        }
    }
    */
}
