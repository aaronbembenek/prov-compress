#include "graph_dummy.hh"
#include "metadata.hh"
#include "helpers.hh"

#include <iostream>

Graph_Dummy::Graph_Dummy(DummyMetadata* metadata) : metadata_(metadata) {
    map<string, string> node_md;
    string typ, head, tail;
    vector<string> dests;
    
    vector<string> identifiers = metadata_->get_ids();

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
            graph_[head].push_back(tail);
            tgraph_[tail].push_back(head);
		} else {
            // make sure we have an entry for every identifier
            graph_[id];
            tgraph_[id];
        }
	}
}

vector<Node_Id> Graph_Dummy::get_outgoing_edges(Node_Id node) {
    
}

vector<Node_Id> Graph_Dummy::get_incoming_edges(Node_Id node) {
}

size_t Graph_Dummy::get_node_count() {
    return 0;
}

Graph_Dummy::~Graph_Dummy() {}
