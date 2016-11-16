#include "helpers.hh"

size_t nbits_for_int(int i) {
    assert(i >= 0);
    return floor(log(max(i, 1))/log(2)) + 1;
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

string remove_char(string str, char ch) {
    str.erase(remove(str.begin(), str.end(), ch), str.end());
    return str;
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
