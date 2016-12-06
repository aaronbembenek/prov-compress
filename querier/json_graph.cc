#include "json_graph.hh"
#include "json/json.h"

JsonGraph::JsonGraph(string& infile) {
    Json::Reader reader;
    Json::FastWriter fastWriter;
    Json::Value root;
    Json::Value json_typ;
    size_t pos;
    size_t ctr = 0;
    vector<string> relation_ids;

    ifstream input(infile);
    for (string line; getline(input, line);) {
        string json;
        if ((pos = line.find(DICT_BEGIN)) == string::npos) {
            continue;
        }
        json = line.substr(pos);
        bool parsingSuccessful = reader.parse( json, root );
        if ( !parsingSuccessful ) {
            // report to the user the failure and their locations in the document.
            std::cout  << "Failed to parse configuration\n"
                       << reader.getFormattedErrorMessages();
            return;
        }
        for (auto typ : typs) {
            if (typ == "prefix") {
                continue;
            }
            json_typ = root.get(typ, "None");
            if (json_typ != "None") {
                auto ids = json_typ.getMemberNames();
                for (auto id : ids) {
                    json_typ[id]["typ"] = typ;
                    id2jsonstr[id] = fastWriter.write(json_typ[id]);
                    // set identifier to id mapping
                    nodeid2id[ctr] = id;
                    id2nodeid[id] = ctr++;
                    
                    // count number of nodes
                    if (!RELATION_TYPS.count(typ)) {
                        node_ids.push_back(id);
                    } else {
                        relation_ids.push_back(id);
                    }
                }
            }
        }
    }
    construct_graph();
}

vector<Node_Id> JsonGraph::get_outgoing_edges(Node_Id node) {
        return tgraph_[node];
}

vector<Node_Id> JsonGraph::get_incoming_edges(Node_Id node) {
        return graph_[node];
}

size_t JsonGraph::get_node_count() {
        return graph_.size();
}

vector<Node_Id> JsonGraph::friends_of(Node_Id pathname, Node_Id executable) {
    (void)(pathname);
    (void)(executable);
    return {};
}

void JsonGraph::construct_graph() {
    Node_Id ctr = 0;
    string typ, head, tail;

    for (auto id : node_ids) {
        graph_[ctr] = {};
        tgraph_[ctr];
        assert(graph_.count(ctr));
        assert(tgraph_.count(ctr));
        nodeid2id[ctr] = id;
        id2nodeid[id] = ctr++;
    }
    for (auto id : relation_ids) {
        auto node_md = get_metadata(id);
        typ = node_md["typ"];
        if (typ == "used") {
            head = node_md["prov:entity"];
            tail = node_md["prov:activity"];
        } else if (typ == "wasGeneratedBy") {
            head = node_md["prov:activity"];
            tail = node_md["prov:entity"];
        } else if (typ == "wasDerivedFrom") {
            head = node_md["prov:usedEntity"];
            tail = node_md["prov:generatedEntity"];
        } else if (typ == "wasInformedBy") {
            head = node_md["prov:informant"];
            tail = node_md["prov:informed"];
        } else {
            assert(typ == "relation");
            head = node_md["cf:sender"];
            tail = node_md["cf:receiver"];
        }
        if (!id2nodeid.count(head)) {
            graph_[ctr] = {};
            tgraph_[ctr] = {};
            nodeid2id[ctr] = head;
            id2nodeid[head] = ctr++;
        }
        if (!id2nodeid.count(tail)) {
            graph_[ctr] = {};
            tgraph_[ctr] = {};
            nodeid2id[ctr] = tail;
            id2nodeid[tail] = ctr++;
        }
        graph_[id2nodeid[head]].push_back(id2nodeid[tail]);
        tgraph_[id2nodeid[tail]].push_back(id2nodeid[head]);
        nodeid2id[ctr] = id;
        id2nodeid[id] = ctr++;
	}
}

map<string, string> JsonGraph::get_metadata(string& identifier) {
    map<string, string> m;
    Json::Reader reader;
    Json::FastWriter fastWriter;
    Json::Value root;

    if(id2jsonstr.count(identifier) == 0) {
        return m;
    }

    bool parsingSuccessful = reader.parse( id2jsonstr[identifier], root );
    if ( !parsingSuccessful ) {
        // report to the user the failure and their locations in the document.
        std::cout  << "Failed to parse configuration\n"
                   << reader.getFormattedErrorMessages();
        return m;
    }
    auto keys = root.getMemberNames();
    for (auto k: keys) {
        string val = fastWriter.write(root[k]);
        remove_char(val, '"'); 
        m[k] = val.substr(0, val.length()-1);
    }
    return m;
}
Node_Id JsonGraph::get_node_id(string identifier) { return id2nodeid[identifier]; }
string JsonGraph::get_identifier(Node_Id node) { return nodeid2id[node]; }
vector<string> JsonGraph::get_node_ids() { return node_ids; }

vector<string> JsonGraph::typs = {"prefix", "activity", "relation", "entity", "agent", "message", "used", "wasGeneratedBy", "wasInformedBy", "wasDerivedFrom","unknown"};
