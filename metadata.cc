#include "metadata.hh"

Metadata::Metadata(string& infile) {
    construct_identifiers_dict();
    construct_prov_dicts();
    construct_metadata_dict(infile);
}

void Metadata::construct_identifiers_dict() {
    string buffer;

    read_file(IDENTIFIERS_FILE, buffer);

    string substr = buffer.substr(0, 32);
    string rest = buffer.substr(32);
    BitSet bs(substr);
    bs.get_bits(num_nodes, 32, 0);
    id_bits = nbits_for_int(num_nodes);

    identifiers = split(rest, ',');
    for (size_t i = 0; i < num_nodes; ++i) {
        // not yet set for relations
        intid2id[i] = identifiers[i]; 
        id2intid[identifiers[i]] = i; 
    }
}

void Metadata::construct_prov_dicts() {
    string buffer;
    vector<string> data;

    read_file(PROV_DICTS_FILE, buffer);
    data = split(buffer, DICT_END);
    assert(data.size() == 5);

    vector<map<unsigned char, string>*> dicts = {&key_dict, &val_dict, &prov_label_dict, &typ_dict, &node_types_dict};
    vector<size_t*> bits = {&key_bits, &val_bits, &label_bits, &typ_bits, &node_type_bits};
    val_bits = max(val_bits, node_type_bits);
    for (size_t i = 0; i < data.size(); ++i) {
        set_dict_entries(*dicts[i], data[i], 2);
        *bits[i] = nbits_for_int(dicts[i]->size());
    }
}

void Metadata::construct_metadata_dict(string& infile) {
    string buffer;
    read_file(infile, buffer);
    metadata_bs = new BitSet(buffer); 
    size_t total_size, cur_pos, val_size;
    unsigned char key, encoded_val;
    string str_val;

    size_t num_equal_keys, num_encoded_keys, num_other_keys;
    vector<size_t*> num_key_types = {&num_equal_keys, &num_encoded_keys, &num_other_keys};

    // get the total size of the data
    cur_pos = 0;
    metadata_bs->get_bits<size_t>(total_size, 32, cur_pos);
    cur_pos += 32;

    // get defaults
    vector<map<string, string>*> default_data_dicts = {&default_node_data, &default_relation_data};
    for (map<string, string>* default_data: default_data_dicts) {
        for (size_t* key : num_key_types) {
            metadata_bs->get_bits<size_t>(*key, key_bits, cur_pos);
            cur_pos += key_bits;
        }
        // there should be no keys equal to the default since we're encoding the default
        assert(num_equal_keys == 0);
        // deal with encoded values
        for (size_t i = 0; i < num_encoded_keys; ++i) {
            metadata_bs->get_bits<unsigned char>(key, key_bits, cur_pos);
            cur_pos += key_bits;
            
            // we encode cf:type as an integer if it is a node
            metadata_bs->get_bits<unsigned char>(encoded_val, val_bits, cur_pos);
            if (key_dict[key] == "cf:type" && (default_data != &default_relation_data)) {
                (*default_data)[key_dict[key]] = node_types_dict[encoded_val];
            } else {
                (*default_data)[key_dict[key]] = val_dict[encoded_val];
            }
            cur_pos += val_bits;
        }
        // nonencoded values
        for (size_t i = 0; i < num_other_keys; ++i) {
            metadata_bs->get_bits<unsigned char>(key, key_bits, cur_pos);
            cur_pos += key_bits;
            
            metadata_bs->get_bits<size_t>(val_size, MAX_STRING_SIZE_BITS, cur_pos);
            cur_pos += MAX_STRING_SIZE_BITS;
         
            metadata_bs->get_bits_as_str(str_val, val_size, cur_pos);
            cur_pos += val_size;
            (*default_data)[key_dict[key]] = str_val;
        }
    }

    // go through all node data, creating a map from intid to index of string 
    for (size_t i = 0; i < num_nodes; ++i) {
        intid2dataindex[i] = cur_pos;
        
        cur_pos += typ_bits;
        for (size_t* key : num_key_types) {
            metadata_bs->get_bits<size_t>(*key, key_bits, cur_pos);
            cur_pos += key_bits;
        }
        cur_pos += key_bits*num_equal_keys;
        cur_pos += (key_bits+val_bits)*num_encoded_keys;
        for (size_t i = 0; i < num_other_keys; ++i) {
            cur_pos += key_bits;
            metadata_bs->get_bits<size_t>(val_size, MAX_STRING_SIZE_BITS, cur_pos);
            cur_pos += MAX_STRING_SIZE_BITS;
            cur_pos += val_size;
        }
    }
    // go through all relation data, creating a map from intid to index of string 
    int relation_id;
    int num_relations = 0;
    while(cur_pos < total_size) {
        metadata_bs->get_bits<int>(relation_id, 2*id_bits, cur_pos);
        cur_pos += 2*id_bits;

        intid2dataindex[relation_id] = cur_pos;
        id2intid[identifiers[num_nodes + num_relations]] = relation_id;
        intid2id[relation_id] = identifiers[num_nodes + num_relations];
        num_relations++;
        
        cur_pos += typ_bits;
        for (size_t* key : num_key_types) {
            metadata_bs->get_bits<size_t>(*key, key_bits, cur_pos);
            cur_pos += key_bits;
        }
        cur_pos += key_bits*num_equal_keys;
        cur_pos += (key_bits+val_bits)*num_encoded_keys;
        for (size_t i = 0; i < num_other_keys; ++i) {
            cur_pos += key_bits;
            metadata_bs->get_bits<size_t>(val_size, MAX_STRING_SIZE_BITS, cur_pos);
            cur_pos += MAX_STRING_SIZE_BITS;
            cur_pos += val_size;
        }
    }

    assert(cur_pos == total_size);
}

map<string, string> Metadata::get_metadata(string& identifier) {
    map<string, string> metadata;
    size_t cur_pos, val_size;
    unsigned char key, encoded_val, typ;
    string str_val;

    size_t num_equal_keys, num_encoded_keys, num_other_keys;
    vector<size_t*> num_key_types = {&num_equal_keys, &num_encoded_keys, &num_other_keys};

    //assert(id2intid.count();
    auto my_intid = id2intid.find(identifier);
    if (my_intid == id2intid.end()) {
        return metadata;
    }

    cur_pos = intid2dataindex[my_intid->second];

    // get type
    metadata_bs->get_bits<unsigned char>(typ, typ_bits, cur_pos);
    cur_pos += typ_bits;
    metadata["typ"] = typ_dict[typ];
    bool is_relation = (typ_dict[typ] == "relation");

    // get sender/receiver if a relation
    if (is_relation) {
        metadata["cf:sender"] = intid2id[my_intid->second >> id_bits];
        metadata["cf:receiver"] = intid2id[my_intid->second & ((1 << id_bits) - 1)]; 
    }

    // get the number of each type of key
    for (size_t* key : num_key_types) {
        metadata_bs->get_bits<size_t>(*key, key_bits, cur_pos);
        cur_pos += key_bits;
    }

    // add equal keys
    for (size_t i = 0; i < num_equal_keys; ++i) {
        metadata_bs->get_bits<unsigned char>(key, key_bits, cur_pos);
        cur_pos += key_bits;
        if (is_relation) {
            metadata[key_dict[key]] = default_relation_data[key_dict[key]];
        } else {
            // we can't encode because we don't know if this node is relative or not
            metadata[key_dict[key]] = "=";
        }
    }

    // unencode encoded values
    for (size_t i = 0; i < num_encoded_keys; ++i) {
        metadata_bs->get_bits<unsigned char>(key, key_bits, cur_pos);
        cur_pos += key_bits;
        metadata_bs->get_bits<unsigned char>(encoded_val, val_bits, cur_pos);
        cur_pos += val_bits;
        if (key_dict[key] == "cf:type" && !is_relation) {
            metadata[key_dict[key]] = node_types_dict[encoded_val];
        }
        else {
            metadata[key_dict[key]] = val_dict[encoded_val];
        }
    }

    // nonencoded values
    for (size_t i = 0; i < num_other_keys; ++i) {
        metadata_bs->get_bits<unsigned char>(key, key_bits, cur_pos);
        cur_pos += key_bits;
        metadata_bs->get_bits<size_t>(val_size, MAX_STRING_SIZE_BITS, cur_pos);
        cur_pos += MAX_STRING_SIZE_BITS;
        metadata_bs->get_bits_as_str(str_val, val_size, cur_pos);
        cur_pos += val_size;
        /* TODO
        if (key_dict[key] == "prov:label"):
            str_val.substr(0, label_bits)
        */
        metadata[key_dict[key]] = str_val; 
    }

    // we're done if this was a relation
    if (is_relation) {
        return metadata;
    }

    // else we need to encode equal keys in relation to another node or default
    auto relative = metadata.find(RELATIVE_NODE);
    map<string, string> relative_metadata;
    if (relative != metadata.end() && relative->second != "=" && stoi(relative->second) != my_intid->second) {
        // the node was encoded in relation to another node
        relative_metadata = get_metadata(intid2id[stoi(relative->second)]);
    } else {
        // the node was encoded in relation to the default data
        relative_metadata = default_node_data; 
    }
    for (auto kv: metadata) {
        if (kv.second == "=") {
            metadata[kv.first] = default_node_data[kv.first];
        }
    }
    return metadata;
}

const string Metadata::PROV_DICTS_FILE = "prov_data_dicts.txt";
const string Metadata::IDENTIFIERS_FILE = "identifiers.txt";
const string Metadata::METADATA_FILE = "compressed_metadata.txt";
const string Metadata::RELATIVE_NODE = "@";
const string Metadata::DEFAULT_NODE_KEY = "-1";
const string Metadata::DEFAULT_RELATION_KEY = "-2";


