#ifndef META_H
#define META_H

#include "helpers.hh"
#include "graph.hh"

class Metadata {
public:
    static const set<string> RELATION_TYPS;
    size_t num_nodes;

    vector<string> identifiers;
    virtual map<string, string> get_metadata(string& identifier) = 0;
    virtual Node_Id get_node_id(string) = 0;
    virtual string get_identifier(Node_Id) = 0;
    virtual vector<string> get_node_ids() = 0;
};

class CompressedMetadata : public Metadata {
private:
    // hardcoded constants/dictionaries
    static const string COMMONSTR_FILE;
    static const string PROV_DICTS_FILE;
    static const string IDENTIFIERS_FILE;
    static const string RELATIVE_NODE;
    static const int MAX_STRING_SIZE_BITS = 10;
    static const int MAX_COMMON_STRS = 300;

    static const vector<size_t> DATE_BITS;
    static const size_t DATE_TYPE_BITS;
    static const size_t COMMONSTR_BITS;

    // prov strings dictionaries (constructed from file)
    map<unsigned char, string>typ_dict;
    map<unsigned char, string>key_dict;
    map<unsigned char, string>prov_label_dict;
    map<unsigned char, string>val_dict;
    map<int, string>commonstr_dict;

    map<string, string>default_node_data;
    map<string, string>default_relation_data;
    vector<size_t>default_date;

    size_t typ_bits;
    size_t key_bits;
    size_t label_bits;
    size_t val_bits;
    size_t id_bits;
    size_t date_type_bits;

    map<string, Node_Id>id2nodeid;
    map<Node_Id, string>nodeid2id;
    
    map<Node_Id, size_t>nodeid2dataindex;
    BitSet* metadata_bs;

public:
    CompressedMetadata(string& infile);
    map<string, string> get_metadata(string& identifier) override;
    Node_Id get_node_id(string) override;
    string get_identifier(Node_Id) override;

private: // helper functions
    void construct_identifiers_dict();
    void construct_prov_dicts();
    void construct_commonstr_dict();
    size_t find_next_entry(size_t cur_pos);
    void construct_metadata_dict(string& infile);
    vector<string> get_node_ids() override;
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
