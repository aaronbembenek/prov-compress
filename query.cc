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
#define UNKNOWN '?'
#define DEFAULT_DATA "-1"


/*
    direct_ancestor(identifier--the thing in red) => return identifier
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

int main(int argc, char *argv[]) {
    ifstream infile;
    string buffer;
    vector<string> data;

    if (argc == 1) {
        infile.open("compressed.out", ios::in);
    } else if (argc == 2) {
        infile.open(argv[1], ios::in);
    } else {
        cout << "Usage: ./query [infile]" << endl;;
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
  
    map<string, string> iti_dict;
    for (auto i = iti_vector.begin(); i != iti_vector.end(); ++i) {
        size_t pos = (*i).find(KEY_VAL_SEP);
        iti_dict[(*i).substr(0,pos)] = (*i).substr(pos+1);
    }

    map<string, vector<string>> nodes_data;
    nodes_data[DEFAULT_DATA] = split(nodes_vector[0], VALUES_SEP);
    for (auto i = nodes_vector.begin()+1; i != nodes_vector.end(); ++i) {
        auto data = split(*i, VALUES_SEP);
        nodes_data[data[0]] = data;
    }

    map<string, vector<string>> relations_data;
    relations_data[DEFAULT_DATA] = split(relations_vector[0], VALUES_SEP);
    for (auto i = relations_vector.begin()+1; i != relations_vector.end(); ++i) {
        auto data = split(*i, VALUES_SEP);
        relations_data[data[0]] = data;
    }


    /*
    None,457525,4,1516773907,734854839,9,2016:11:03T22:07:06.978,[]#
    2,0,0,0,0,0,0,0,cf:gid$1000,cf:uid$1000,prov:label$[task] 9#
    4,?#
    42,?#
    11,5,0,0,0,1,2016:11:03T22:07:09.119,0,cf:gid$1000,cf:uid$1000,prov:label$[task] 10#
    35,?#
    19@4,0,0,0,0,2,0,0,0,0,prov:label$[task] 11#
    */
    return 0;
}
