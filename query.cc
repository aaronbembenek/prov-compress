#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include<sstream>
#include<algorithm>

#define DICT_BEGIN '{'
#define DICT_END '}'
#define EXTRA_KEY '$'
#define RELATIVE_NODE '@'
#define VALUES_SEP ','
#define IDENTIFIER_SEP '#'
#define KEY_VAL_SEP ':'

using namespace std;

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
    infile.close();

    for (auto i = data.begin(); i != data.end(); ++i) {
        *i = remove_char(*i, DICT_BEGIN);
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
