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
#include<bitset>

#define DICT_BEGIN '{'
#define DICT_END '}'
#define RELATIVE_NODE '@'
#define DEFAULT_NODE_KEY "-1"
#define DEFAULT_RELATION_KEY "-2"

using namespace std;

/* BITSTR HELPERS */
size_t nbits_for_int(int i);
bool str_to_int(string s, int& i, int val_type_base);

/* STRING HELPERS */
vector<string> split(string& str, char delim);
vector<string> split(string& str, string& delim);
string remove_char(string str, char ch);

/* DICTIONARY HELPERS */
template <typename K, typename V>
void print_dict(map<K, V>& dict);
template <typename K>
void set_dict_entries(map<K, string>& dict, string str, int val_type_base);

template <typename K, typename V>
void print_dict(map<K, V>& dict) {
    for (auto i = dict.begin(); i != dict.end(); ++i) {
        cout << i->first << " => " << i->second << endl;
    }
}

template <typename K>
void set_dict_entries(map<K, string>& dict, string str, int val_type_base) {
    str = remove_char(str, DICT_BEGIN);
    str = remove_char(str, DICT_END);
    str = remove_char(str, '\'');
    auto keyvals = split(str, ',');
    for (auto kv = keyvals.begin(); kv != keyvals.end(); ++kv) {
        string delim = ": ";
        auto pair = split(*kv, delim); 

        int key;
        assert(str_to_int(pair[1], key, val_type_base));
        dict[key] = pair[0];
    }
}

/*
 * Given a string (std::string), returns an object that allows one to index
 * into arbitrary bit locations of the string and read x number of bits
 * from that index.
 */
class BitSet {
    static const int mask = (1<<3) - 1;

public:
    // constructor
    BitSet(string& s) {
        for (unsigned i = 0; i < s.length(); ++i) {
            bitsets_.push_back(bitset<8>(s[i]));
        }
    }

    bool get_bit(size_t pos) {
        size_t char_pos = (pos >> 3);
        size_t offset = (pos & mask);
        return bitsets_[char_pos][7-offset];
    }

    template <typename T>
    void get_bits(T& val, size_t num_bits, size_t pos) {
        assert(num_bits <= sizeof(T)*8);

        val = 0;
        for (size_t i = 0; i < num_bits; ++i) {
            //cout << pos+i << " " << get_bit(pos+i) << endl;
            val |= get_bit(pos+i);
            if (i != num_bits-1)
                val <<= 1;
        }
        //cout << bitset<32>(val).to_string() << endl;
    }

    // specialize for strings
    void get_bits_as_str(string& str, size_t num_bits, size_t pos) {
        if ((num_bits & mask) != 0) {
            int i = 0;
            get_bits<int>(i, 32, pos);
            cout << bitset<32>(i).to_string() << endl;
        }
        assert((num_bits & mask) == 0); // must be a multiple of 8

        string s = "";
        for (size_t i = 0; i < (num_bits >> 3); ++i) {
            bitset<8> b(0);
            for (size_t j = 0; j < 1<<3; ++j) {
                b[7-j] = get_bit(pos + (i << 3) + j);
            }
            s += static_cast<unsigned char>(b.to_ulong());
        }
        str = s;
    }

private: 
    std::vector<bitset<8>> bitsets_;
};

#endif /*HELPERS_H*/
