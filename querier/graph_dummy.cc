#include "graph_dummy.hh"
#include "metadata.hh"
#include "helpers.hh"

#include <iostream>

DummyGraph::DummyGraph(Metadata* metadata) : metadata_(metadata) {
    map<string, string> node_md;
    string typ, head, tail;
    
    vector<string> identifiers = metadata_->identifiers;

    for (auto id : identifiers) {
        node_md = metadata_->get_metadata(id);
        auto typ_it = metadata_->RELATION_TYPS.find(node_md["typ"]);
        if (typ_it != metadata_->RELATION_TYPS.end()) {
            typ = *typ_it;
			if (typ == "used") {
				head = node_md["prov:entity"];
				tail = node_md["prov:activity"];
			}
			else if (typ == "wasGeneratedBy") {
				head = node_md["prov:activity"];
				tail = node_md["prov:entity"];
			}
			else if (typ == "wasDerivedFrom") {
				head = node_md["prov:usedEntity"];
				tail = node_md["prov:generatedEntity"];
			}
			else if (typ == "wasInformedBy") {
				head = node_md["prov:informant"];
				tail = node_md["prov:informed"];
			}
			else if (typ == "relation") {
				head = node_md["cf:sender"];
				tail = node_md["cf:receiver"];
			}
            graph_[metadata_->get_node_id(head)].push_back(metadata_->get_node_id(tail));
            tgraph_[metadata_->get_node_id(tail)].push_back(metadata_->get_node_id(head));
		} else {
            // make sure we have an entry for every identifier
            graph_[metadata_->get_node_id(id)];
            tgraph_[metadata_->get_node_id(id)];
        }
	}
    for (auto p: graph_) {
        cout << p.first << " " << (p.second).size() << endl;
    }
}

vector<Node_Id> DummyGraph::get_outgoing_edges(Node_Id node) {
    return graph_[node];
}

vector<Node_Id> DummyGraph::get_incoming_edges(Node_Id node) {
    return tgraph_[node];
}

size_t DummyGraph::get_node_count() {
    return metadata_->num_nodes;
}

DummyGraph::~DummyGraph() {}
