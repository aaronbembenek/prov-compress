#ifndef HELPERS_H
#define HELPERS_H

#include<iostream>
#include<fstream>
#include<string>
#include<cstring>
#include<vector>
#include<map>
#include<set>
#include<sstream>
#include<algorithm>
#include<cassert>
#include<cmath>
#include<bitset>
#include <chrono> 

#define NUM_REPS 50

#define DICT_BEGIN '{'
#define DICT_END '}'

using namespace std;

typedef size_t Node_Id;

// Dictionaries for identifiers
extern map<string, int> id2intid;
extern map<int, string> intid2id;

/* BENCHMARKING HELPERS */
template<typename TimeT = std::chrono::nanoseconds>
struct exec_stats {
    int vm_usage;
    typename TimeT::rep time;
};
int parse_stats_line(char* line);
int virtualmem_usage();

template<typename TimeT = std::chrono::nanoseconds>
struct measure
{
    template<typename T, typename F, typename ...Args>
    static typename TimeT::rep execution(T& obj, F&& func, Args&&... args)
    {
        auto start = std::chrono::steady_clock::now();
        for (auto i = 0; i < NUM_REPS; ++i) {
            (obj.*func)(std::forward<Args>(args)...);
        }
        auto duration = std::chrono::duration_cast< TimeT> (std::chrono::steady_clock::now() - start);
        return duration.count();
    }
};


/* BITSTR HELPERS */
size_t nbits_for_int(int i);
bool str_to_int(string s, int& i, int val_type_base);

/* STRING HELPERS */
vector<string> split(string& str, char delim);
vector<string> split(string& str, string& delim);
void remove_char(string& str, char ch);
bool non_ascii(char c);
void remove_non_ascii(string& str);

    /* DICTIONARY HELPERS */
string read_file(string filename, string& str);
template <typename K, typename V>
void print_dict(map<K, V>& dict);
void print_str_vector(vector<string> v);
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
    remove_char(str, DICT_BEGIN);
    remove_char(str, DICT_END);
    remove_char(str, '\'');
    auto keyvals = split(str, ',');
    for (auto kv = keyvals.begin(); kv != keyvals.end(); ++kv) {
        string delim = ": ";
        auto pair = split(*kv, delim); 

        int key;
        assert(str_to_int(pair[1], key, val_type_base));
        remove_char(pair[0], ' ');
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
    size_t get_bits(T& val, size_t num_bits, size_t pos) {
        assert(num_bits <= sizeof(T)*8);

        val = 0;
        for (size_t i = 0; i < num_bits; ++i) {
            val |= get_bit(pos+i);
            if (i != num_bits-1)
                val <<= 1;
        }

        return num_bits;
    }

    // specialize for strings
    void get_bits_as_str(string& str, size_t num_bits, size_t pos) {
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
