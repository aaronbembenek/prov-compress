#ifndef META_H
#define META_H

#include "helpers.hh"
#include "graph.hh"

class MetadataInterface {
    virtual map<string, string> get_metadata(string& identifier) = 0;
};

class DummyMetadata : MetadataInterface {
    static vector<string> typs;
private:
    map<string, string> id2jsonstr;

public:
    DummyMetadata(string& infile);
    map<string, string> get_metadata(string& identifier) override;
};


class CompressedMetadata : MetadataInterface {
private:
    // hardcoded constants/dictionaries
    static const string PROV_DICTS_FILE;
    static const string IDENTIFIERS_FILE;
    static const string RELATIVE_NODE;
    static const int MAX_STRING_SIZE_BITS = 10;
    static const set<string> RELATION_TYPS;

    static const vector<size_t> DATE_BITS;
    static const size_t DATE_TYPE_BITS;

    // prov strings dictionaries (constructed from file)
    map<unsigned char, string>node_types_dict;
    map<unsigned char, string>typ_dict;
    map<unsigned char, string>key_dict;
    map<unsigned char, string>prov_label_dict;
    map<unsigned char, string>val_dict;
    vector<string> identifiers;
    BitSet* metadata_bs;

    map<string, string>default_node_data;
    map<string, string>default_relation_data;
    vector<size_t>default_date;
    map<int, size_t>intid2dataindex;
    
    map<string, int>id2intid;
    map<int, string>intid2id;

    size_t node_type_bits;
    size_t typ_bits;
    size_t key_bits;
    size_t label_bits;
    size_t val_bits;
    size_t id_bits;
    size_t date_type_bits;
    size_t num_nodes;

public:
    CompressedMetadata(string& infile);
    map<string, string> get_metadata(string& identifier) override;
    Node_Id get_node_id(string& identifier) {
        return (Node_Id) id2intid[identifier];
    }

private: // helper functions
    void construct_identifiers_dict();
    void construct_prov_dicts();
    size_t find_next_entry(size_t cur_pos);
    void construct_metadata_dict(string& infile);
};

/*
class MetadataEntry {
public:
    MetadataEntry(unsigned char typ) : typ_(typ) {};

    void add_equal_key(unsigned char k) {
        equal_keys_.push_back(k);
    }
    void add_encoded_key_val(unsigned char k, unsigned char v) {
        encoded_key_vals_[k] = v;
    }
    void add_other_key_val(unsigned char k, string v) {
        other_key_vals_[k] = v;
    }

    vector<unsigned char> get_keys_equal() {
        return equal_keys_;
    }
    map<unsigned char, unsigned char> get_encoded() {
        return encoded_key_vals_;
    }
    map<unsigned char, string> get_other() {
        return other_key_vals_;
    }

private:
    unsigned char typ_;
    vector<unsigned char> equal_keys_;
    map<unsigned char, unsigned char> encoded_key_vals_;
    map<unsigned char, string> other_key_vals_;
};
*/

#endif
