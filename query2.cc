#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include<map>
#include<sstream>
#include<algorithm>
#include<cassert>

#define DICT_BEGIN '{'
#define DICT_END '}'
#define RELATIVE_NODE '@'
#define VALUES_SEP ','
#define IDENTIFIER_SEP '#'
#define KEY_VAL_SEP '$'
#define UNKNOWN "?"

/*
    SUPPORTED QUERIES:
    direct_ancestor(identifier) => return identifier
    all_ancestors(identifier) => return list of identifiers
    all_descendants(identifier) => return list of identifiers
    all_paths(source, sink) => return list of list of identifiers
    friends(identifier) => return list of identifiers (is this a useful query to support?)
    metadata(identifier) => return (JSON output?) of identifier
 */

using namespace std;

map<string, int> node_keys = {
    {"cf:id", 1},
    {"cf:type", 2},
    {"cf:boot_id", 3},
    {"cf:machine_id", 4},
    {"cf:version", 5},
    {"cf:date", 6},
    {"cf:taint", 7}
};

map<string, int> relation_keys = {
    {"cf:id", 1},
    {"cf:boot_id", 2},
    {"cf:machine_id", 3},
    {"cf:date", 4},
    {"cf:taint", 5},
    {"cf:type", 6},
    {"prov:label", 7},
    {"cf:allowed", 8},
    {"cf:sender", 9},
    {"cf:receiver", 10}
};
map<string, string> id_to_int_dict;
map<string, string> int_to_id_dict;
map<string, vector<string>> nodes_data;
map<string, vector<string>> relations_data;
vector<string> default_node_data;
vector<string> default_relation_data;

void print_dict(map<string, vector<string>>& dict) {
    for (auto i = dict.begin(); i != dict.end(); ++i) {
        cout << i->first << ": " << i->second.size() << endl;
    }
}

vector<string> split(string str, char delim) {
    vector<string> data;
    stringstream ss(str);
    string substring;
    while (getline(ss, substring, delim)) {
        data.push_back(substring);
    }
    return data;
}

string remove_char(string str, char ch) {
    str.erase(remove(str.begin(), str.end(), ch), str.end());
    return str;
}

string decode_from_default(string& identifier, vector<string>& data) {
    string s = "\"" + identifier;
    s += "\": {";
    auto default_data = (data[0].find('n') != string::npos) ? default_node_data : default_relation_data;
    auto keys = (data[0].find('n') != string::npos) ? node_keys : relation_keys;
    
    // deal with node that has no metadata
    if (data[1] == UNKNOWN) {
        return s += "?}";
    }
    for (unsigned i = 1; i < default_data.size(); ++i) {
        size_t pos;
        try {
            int val = stoi(data[i], &pos, 10);
            if (data[i][pos] != '\0') {
                continue;
            }
            // they were equal
            if (!val) {
                data[i] = default_data[i];
            } else {
                // this looks like an int! should be delta encoded off the default
                // we have to ensure that the default val was an int too
                int default_val = stoi(default_data[i], &pos, 10);
                if (default_data[i][pos] != '\0') {
                    continue;
                }
                data[i] = to_string(default_val + val);
            }
        } catch (const invalid_argument&) {
            // it's a string, so just continue
            continue; 
        }
    }

    for (auto i = keys.begin(); i != keys.end(); ++i) {
        if (i->first == "cf:sender" || i->first == "cf:receiver") {
            s += "\"" + i->first + "\": " + int_to_id_dict[data[i->second]] + ",";

        } else {
            s += "\"" + i->first + "\": " + data[i->second] + ",";
        }
    }
    // add the rest of the keys
    for (auto i = keys.size()+1; i < data.size(); ++i) {
        auto pos = data[i].find(KEY_VAL_SEP);
        auto key = data[i].substr(0, pos);
        auto val = data[i].substr(pos+1);
        s += "\"" + key + "\": " + val + ",";
    }
    s += "}";
    return s;
}

string get_metadata(string identifier) {
    auto index = id_to_int_dict[identifier];
    vector<string> data;

    if (index.find('n') != string::npos) {
        // we are dealing with a node
        data = nodes_data[index];
        // determine if we need to go through one more level of indirection
        size_t pos = data[0].find(RELATIVE_NODE);
        if (pos != string::npos) {
            vector<string> relative_node_data = nodes_data[data[0].substr(pos+1)];
            data[0] = data[0].substr(0, pos);
            for (unsigned i = 0; i < data.size(); ++i) {
                try {
                    int val = stoi(data[i]);
                    if (!val) {
                        data[i] = relative_node_data[i];
                    }
                } catch (const invalid_argument&) {continue;}
            }
        }
        return decode_from_default(identifier, data);
    } else {
        // we are dealing with a relation
        data = relations_data[index];
        return decode_from_default(identifier, data);
    }
    assert(0);
    return "";
}

int main(int argc, char *argv[]) {
    ifstream infile;
    string buffer;
    vector<string> data;

    if (argc == 1) {
        infile.open("compressed.out", ios::in);
    } else if (argc == 2) {
        infile.open(argv[1], ios::in);
    } else {
        cout << "Usage: ./query [infile]" << endl;
        return 1;
    }
    if (!infile.is_open()) {
        cout << "Failed to open input file" << endl;
        return 1;
    }
    getline(infile, buffer);
    data = split(buffer, DICT_END);
    assert(data.size() == 3);
    infile.close();

    auto iti_vector = split(remove_char(data[0], DICT_BEGIN), IDENTIFIER_SEP);
    auto nodes_vector = split(remove_char(data[1], DICT_BEGIN), IDENTIFIER_SEP);
    auto relations_vector = split(remove_char(data[2], DICT_BEGIN), IDENTIFIER_SEP);
  
    for (auto i = iti_vector.begin(); i != iti_vector.end(); ++i) {
        size_t pos = i->find(KEY_VAL_SEP);
        id_to_int_dict[i->substr(0,pos)] = i->substr(pos+1);
        int_to_id_dict[i->substr(pos+1)] = i->substr(0,pos);
    }
    default_node_data = split(nodes_vector[0], VALUES_SEP);
    for (auto i = nodes_vector.begin()+1; i != nodes_vector.end(); ++i) {
        auto data = split(*i, VALUES_SEP);
        // make sure that the key is only the identifier int---no @ symbols
        nodes_data[data[0].substr(0,data[0].find(RELATIVE_NODE))] = data;
    }
    default_relation_data = split(relations_vector[0], VALUES_SEP);
    for (auto i = relations_vector.begin()+1; i != relations_vector.end(); ++i) {
        auto data = split(*i, VALUES_SEP);
        relations_data[data[0]] = data;
    }

    for (auto i = id_to_int_dict.begin(); i != id_to_int_dict.end(); ++i) {
        cout << get_metadata(i->first) << endl;
        //get_metadata(i->first);
    }
    return 0;
}
