#include "query.hh"

template <typename K, typename V>
void set_dict_entries(map<K, V>& dict, string str) {
    str = remove_char(str, DICT_BEGIN);
    str = remove_char(str, DICT_END);
    str = remove_char(str, '\'');
    auto keyvals = split(str, ',');
    for (auto kv = keyvals.begin(); kv != keyvals.end(); ++kv) {
        string delim = ": ";
        auto pair = split(*kv, delim); 
        for (char c : pair[1]) {
            assert(c == '0' || c == '1');
            dict[pair[0]] = (dict[pair[0]] << 1) | (c - '0');
        }
    }
}

void construct_prov_dicts() {
    ifstream infile;
    string buffer;
    vector<string> data;

    infile.open(PROV_DICTS_FILE, ios::in);
    assert(infile.is_open());
    getline(infile, buffer);
    infile.close();

    data = split(buffer, DICT_END);
    assert(data.size() == 5);

    std::vector<map<string, unsigned char>> dicts = {key_dict, val_dict, prov_label_dict, typ_dict, node_types_dict};
    for (size_t i = 0; i < data.size(); ++i) {
        // XXX will need the other way around?
        set_dict_entries(dicts[i], data[i]);
    }
}

void construct_identifiers_dict() {
    ifstream infile;
    string buffer;

    infile.open(IDENTIFIERS_FILE, ios::in);
    assert(infile.is_open());
    getline(infile, buffer);
    infile.close();

    set_dict_entries(id_to_intid_dict, buffer);
    // create the dictionary mapping the other direction
    for (auto i = id_to_intid_dict.begin(); i != id_to_intid_dict.end(); ++i) {
        intid_to_id_dict[i->second] = i->first; 
    }
}

string decode_from_default(string& identifier, vector<string>& data) {
    (void)data;
    return identifier;
}

string get_metadata(string identifier) {
    return identifier;
}

int main(int argc, char *argv[]) {
    ifstream infile;
    string buffer;
    vector<string> data;

    if (argc == 1) {
        infile.open("compressed_metadata.txt", ios::in);
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
    infile.close();

    construct_prov_dicts();
    construct_identifiers_dict();
    print_dict(id_to_intid_dict);
    return 0;
}

/*
 * HELPER FUNCTIONS
 */

string remove_char(string str, char ch) {
    str.erase(remove(str.begin(), str.end(), ch), str.end());
    return str;
}

template <typename K, typename V>
void print_dict(map<K, V>& dict) {
    for (auto i = dict.begin(); i != dict.end(); ++i) {
        cout << i->first << ": " << i->second << endl;
    }
}

vector<string> split(string& str, char delim) {
    vector<string> data;
    stringstream ss(str);
    string substring;
    while (getline(ss, substring, delim)) {
        data.push_back(substring);
    }
    return data;
}

vector<string> split(string& str, string& delim) {
    vector<string> data;
    size_t pos = 0;
    string token;
    while ((pos = str.find(delim)) != std::string::npos) {
        token = str.substr(0, pos);
        data.push_back(token);
        str.erase(0, pos + delim.length());
    }    
    data.push_back(str);
    return data;
}

bool bitstr_to_int(string s, int& i) {
    try {
        size_t pos;
        i = stoi(s, &pos, 2);
        // check whether the entire string was an integer
        return (s[pos] == '\0');
    } catch (const invalid_argument&) {
        i = -1;
        return false;
    }
}
