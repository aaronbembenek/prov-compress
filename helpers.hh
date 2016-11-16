#ifndef HELPERS_H
#define HELPERS_H

#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include<map>
#include<sstream>
#include<algorithm>
#include<cassert>
#include<cmath>

#define DICT_BEGIN '{'
#define DICT_END '}'
#define RELATIVE_NODE '@'

using namespace std;

/* BITSTR HELPERS */
size_t nbits_for_int(int i);
bool bitstr_to_int(string s, int& i);

/* STRING HELPERS */
vector<string> split(string& str, char delim);
vector<string> split(string& str, string& delim);
string remove_char(string str, char ch);

/* DICTIONARY HELPERS */
template <typename K, typename V>
void print_dict(map<K, V>& dict);
template <typename K>
void set_dict_entries(map<K, string>& dict, string str);

template <typename K, typename V>
void print_dict(map<K, V>& dict) {
    for (auto i = dict.begin(); i != dict.end(); ++i) {
        cout << i->first << ": " << i->second << endl;
    }
}

template <typename K>
void set_dict_entries(map<K, string>& dict, string str) {
    str = remove_char(str, DICT_BEGIN);
    str = remove_char(str, DICT_END);
    str = remove_char(str, '\'');
    auto keyvals = split(str, ',');
    for (auto kv = keyvals.begin(); kv != keyvals.end(); ++kv) {
        string delim = ": ";
        auto pair = split(*kv, delim); 

        int key;
        assert(bitstr_to_int(pair[0], key));
        dict[key] = pair[1];
    }
}

#endif /*HELPERS_H*/
