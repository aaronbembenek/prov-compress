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
        cout << i->first << " => " << i->second << endl;
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

/*
 * Given a string (std::string), returns an object that allows one to index
 * into arbitrary bit locations of the string and read x number of bits
 * from that index.
 */
class BitSet {

    BitSet(string& s) {
        for (int i = 0; i < s.length(); ++i) {
            bitsets_.push_back(s[i]);
        }
    };

    // returns up to 32 bits starting at pos
    template <typename T>
    void get_bits(T& val, size_t num_bits, size_t pos) {}
 
    template <>
    void get_bits<int>(int& val, size_t num_bits, size_t pos) {
        assert(num_bits <= sizeof(int)*8);
        size_t char_pos = (pos >> 3);
        size_t offset = (pos % 8);

        size_t ctr = num_bits;
        val = 0;
        for (size_t i = 0; i < num_bits; ++i) {
            val |= bitsets_[char_pos+((offset+i) >> 3)][(offset+i)%8];
            val << 1;
        }
    }

private: 
    std::vector<bitset<8>> bitsets_;
};

#endif /*HELPERS_H*/
