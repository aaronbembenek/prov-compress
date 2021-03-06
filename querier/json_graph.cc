#include "json_graph.hh"
#include "json/json.h"

JsonGraph::JsonGraph(string& infile) {
    Json::Reader reader;
    Json::FastWriter fastWriter;
    Json::Value root;
    Json::Value json_typ;
    size_t pos;
    size_t ctr = 0;

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

// Third parameter unused.
map<string, vector<Node_Id>> JsonGraph::friends_of(Node_Id pathname, Node_Id task,
        Metadata*) {
    map<string, vector<Node_Id>> friend_files;

    File_Id file_id = pathname2file[pathname];
    auto task_md = get_metadata(nodeid2id[task]);
    Task_Id task_id = stol(task_md["cf:id"]);
    
    // get the tasks related to this file id
    map<string, set<Task_Id>> relation_tasks = file2tasks[file_id];
    for (auto pair : relation_tasks) {
        string relation = pair.first;
        set<Task_Id> task_ids = pair.second;
        if (!task_ids.count(task_id)) {
            continue;
        }
        // this file is related to the task!
        // get all other files related to the task in the same way as this file id
        for (auto friend_file_id : task2files[task_id][relation]) {
            // we want to record the pathnames (not the file ids)
            friend_files[relation].push_back(file2pathname[friend_file_id]);
        }
    }
    /*
    cout << pathname2file[pathname] << ", " << task_id << ": "; 
    for (auto f: friend_files) {
        cout << f.first << ": ";
        for (auto pathname : f.second) {
            cout << pathname2file[pathname] << " ";
        }
        cout << " || ";
    }
    cout << endl;
    */
    return friend_files;
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

        auto head_md = get_metadata(head);
        auto tail_md = get_metadata(tail);
        assert(tail_md["cf:type"] != "file_name");
        // map from pathname to file id. this should be a one-to-one mapping
        if (head_md["cf:type"] == "file_name") {
            auto path_id = id2nodeid[head];
            auto tail_id = stol(tail_md["cf:id"]);
            assert(!pathname2file.count(path_id)
                    || (pathname2file.count(path_id) 
                        && pathname2file[path_id] == tail_id));
            pathname2file[path_id] = tail_id; 
            file2pathname[tail_id] = path_id;
        }
        // map from file id to sets of tasks (grouped by relation type)
        // we account for dependencies that can go either direction
        else if (head_md["cf:type"] == "file" && tail_md["cf:type"] == "task") {
            auto file_id = stol(head_md["cf:id"]);
            auto task_id = stol(tail_md["cf:id"]);
            auto relation_type = node_md["cf:type"];
            file2tasks[file_id][relation_type].insert(task_id);
            task2files[task_id][relation_type].insert(file_id);
        }
        else if (tail_md["cf:type"] == "file" && head_md["cf:type"] == "task") {
            auto file_id = stol(tail_md["cf:id"]);
            auto task_id = stol(head_md["cf:id"]);
            auto relation_type = node_md["cf:type"];
            file2tasks[file_id][relation_type].insert(task_id);
            task2files[task_id][relation_type].insert(file_id);
        }
        /*
        for (auto pair : file2tasks) {
            auto file_id = pair.first;
            cout << file_id << "\t";
            auto relationids = pair.second;
            for (auto ri : relationids) {
                string relation = ri.first;
                cout << relation << ": ";
                auto ids = ri.second;
                for (auto id : ids) {
                    cout << id << ", ";
                }
            }
            cout << endl;
        }
        */
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
