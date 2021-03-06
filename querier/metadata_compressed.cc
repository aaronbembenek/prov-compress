#include "metadata.hh"

const set<string> Metadata::RELATION_TYPS = {"wasGeneratedBy", "wasInformedBy", "wasDerivedFrom", "used", "relation"};

CompressedMetadata::CompressedMetadata(string& infile) {
    construct_identifiers_dict();
    construct_prov_dicts();
    construct_commonstr_dict();
    construct_metadata_dict(infile);
}

void CompressedMetadata::construct_identifiers_dict() {
    string buffer, default_id, id;
    vector<string> encoded_ids;

    read_file(IDENTIFIERS_FILE, buffer);
    
	string substr = buffer.substr(0, 4);
    string rest = buffer.substr(4);
    BitSet bs(substr);
    bs.get_bits(num_nodes, 32, 0);
    id_bits = nbits_for_int(num_nodes);

    identifiers = split(rest, ',');
    for (size_t i = 0; i < num_nodes; ++i) {
        // not yet set for relations
        nodeid2id[i] = identifiers[i]; 
        id2nodeid[identifiers[i]] = i; 
	}
}

void CompressedMetadata::construct_commonstr_dict() {
    string bin_buffer;
    string str_buffer;
    vector<string> strs;
    int encoding;
    size_t pos = 0;

    read_file(COMMONSTR_FILE+".bin", bin_buffer);
    read_file(COMMONSTR_FILE+".txt", str_buffer);

    strs = split(str_buffer, ',');
    
    BitSet bs(bin_buffer);
    for (unsigned i = 0; i < strs.size(); ++i) { 
        bs.get_bits(encoding, COMMONSTR_BITS, pos);
        commonstr_dict[encoding] = strs[i];
        pos += COMMONSTR_BITS;
    }
}

void CompressedMetadata::construct_prov_dicts() {
    string buffer;
    vector<string> data;

    read_file(PROV_DICTS_FILE, buffer);
    data = split(buffer, DICT_END);
    assert(data.size() == 4);

    vector<map<unsigned char, string>*> dicts = {
        &key_dict, 
        &val_dict, 
        &prov_label_dict, 
        &typ_dict, 
    };
    vector<size_t*> bits = {&key_bits, &val_bits, &label_bits, &typ_bits};
    for (size_t i = 0; i < data.size(); ++i) {
        set_dict_entries(*dicts[i], data[i], 2);
        *bits[i] = nbits_for_int(dicts[i]->size());
    }
    label_bits = 8; // we replace labels with one-byte chars
}

size_t CompressedMetadata::find_next_entry(size_t cur_pos) {
    size_t num_equal_keys, num_encoded_keys, num_common_keys, num_other_keys, num_diff_dates;
    vector<size_t*> num_key_types = {
        &num_equal_keys, 
        &num_encoded_keys, 
        &num_common_keys, 
        &num_other_keys, 
        &num_diff_dates
    };

    size_t date_index, val_size;
    cur_pos += typ_bits;
    for (size_t* key : num_key_types) {
        metadata_bs->get_bits<size_t>(*key, key_bits, cur_pos);
        cur_pos += key_bits;
    }
    cur_pos += key_bits*num_equal_keys;
    cur_pos += (key_bits+val_bits)*num_encoded_keys;
    cur_pos += (key_bits+COMMONSTR_BITS)*num_common_keys;

    for (size_t i = 0; i < num_other_keys; ++i) {
        unsigned char key;
        metadata_bs->get_bits<unsigned char>(key, key_bits, cur_pos);
        cur_pos += key_bits;
        metadata_bs->get_bits<size_t>(val_size, MAX_STRING_SIZE_BITS, cur_pos);
        cur_pos += MAX_STRING_SIZE_BITS;
        
        string str_val;
        metadata_bs->get_bits_as_str(str_val, val_size, cur_pos);
        cur_pos += val_size;
    }

    for (size_t i = 0; i < num_diff_dates; ++i) {
        metadata_bs->get_bits<size_t>(date_index, DATE_TYPE_BITS, cur_pos);
        cur_pos += DATE_TYPE_BITS;
        cur_pos += DATE_BITS[date_index];
    }
    return cur_pos;
}

void CompressedMetadata::construct_metadata_dict(string& infile) {
    string buffer;
    read_file(infile, buffer);
    metadata_bs = new BitSet(buffer); 
    size_t total_size, cur_pos, val_size;
    unsigned char key, encoded_val;
    int common_val;
    string str_val;
    int int_val;

    size_t num_equal_keys, num_encoded_keys, num_common_keys, num_other_keys, num_diff_dates;
    vector<size_t*> num_key_types = {
        &num_equal_keys, 
        &num_encoded_keys, 
        &num_common_keys, 
        &num_other_keys, 
        &num_diff_dates
    };

    // get the total size of the data
    cur_pos = 0;
    metadata_bs->get_bits<size_t>(total_size, 32, cur_pos);
    cur_pos += 32;

    // get default time
    for (auto date_bits: DATE_BITS) {
        metadata_bs->get_bits<int>(int_val, date_bits, cur_pos);
        cur_pos += date_bits;
        default_date.push_back(int_val);
    }
    
    // get default metadata
    vector<map<string, string>*> default_data_dicts = {&default_node_data, &default_relation_data};
    for (map<string, string>* default_data: default_data_dicts) {
        for (size_t* key : num_key_types) {
            metadata_bs->get_bits<size_t>(*key, key_bits, cur_pos);
            cur_pos += key_bits;
        }
        // there should be no keys equal to the default since we're encoding the default
        assert(num_equal_keys == 0);
        // there should also be no date...
        assert(num_diff_dates == 0);
        
        // deal with encoded values
        for (size_t i = 0; i < num_encoded_keys; ++i) {
            metadata_bs->get_bits<unsigned char>(key, key_bits, cur_pos);
            cur_pos += key_bits;
            
            metadata_bs->get_bits<unsigned char>(encoded_val, val_bits, cur_pos);
            (*default_data)[key_dict[key]] = val_dict[encoded_val];
            cur_pos += val_bits;
        }
        
        // deal with common values
        for (size_t i = 0; i < num_common_keys; ++i) {
            metadata_bs->get_bits<unsigned char>(key, key_bits, cur_pos);
            cur_pos += key_bits;
            
            metadata_bs->get_bits<int>(common_val, COMMONSTR_BITS, cur_pos);
            (*default_data)[key_dict[key]] = commonstr_dict[common_val];
            cur_pos += COMMONSTR_BITS;
        }

        // nonencoded values
        char label_key;
        for (size_t i = 0; i < num_other_keys; ++i) {
            metadata_bs->get_bits<unsigned char>(key, key_bits, cur_pos);
            cur_pos += key_bits;
            
            metadata_bs->get_bits<size_t>(val_size, MAX_STRING_SIZE_BITS, cur_pos);
            cur_pos += MAX_STRING_SIZE_BITS;
         
            metadata_bs->get_bits_as_str(str_val, val_size, cur_pos);
            cur_pos += val_size;
            (*default_data)[key_dict[key]] = str_val;

            if (key_dict[key] == "prov:label") {
                label_key = str_val[0];
                str_val = prov_label_dict[label_key] + str_val.substr(1) ;
            }
        }
    }

    // go through all node data, creating a map from nodeid to index of string 
    for (size_t i = 0; i < num_nodes; ++i) {
        nodeid2dataindex[i] = cur_pos;
        cur_pos = find_next_entry(cur_pos);
    }
    // go through all relation data, creating a map from nodeid to index of string 
    int relation_id;
    int num_relations = 0;
    while(cur_pos < total_size) {
        metadata_bs->get_bits<int>(relation_id, 2*id_bits, cur_pos);
        cur_pos += 2*id_bits;

        nodeid2dataindex[relation_id] = cur_pos;

        cur_pos = find_next_entry(cur_pos);

        id2nodeid[identifiers[num_nodes + num_relations]] = relation_id;
        nodeid2id[relation_id] = identifiers[num_nodes + num_relations];
        num_relations++;
    }
    assert(cur_pos == total_size);
}

map<string, string> CompressedMetadata::get_metadata(string& identifier) {
    map<string, string> metadata;
    size_t cur_pos, val_size, date_index;
    unsigned char key, encoded_val, typ;
    int common_val;
    int int_val, nodeid;
    string str_val;

    size_t num_equal_keys, num_encoded_keys, num_common_keys, num_other_keys, num_diff_dates;
    vector<size_t*> num_key_types = {
        &num_equal_keys, 
        &num_encoded_keys, 
        &num_common_keys, 
        &num_other_keys, 
        &num_diff_dates
    };

    auto my_nodeid = id2nodeid.find(identifier);
    if (my_nodeid == id2nodeid.end()) {
        return metadata;
    }
    cur_pos = nodeid2dataindex[my_nodeid->second];

    // get type
    metadata_bs->get_bits<unsigned char>(typ, typ_bits, cur_pos);
    cur_pos += typ_bits;
    metadata["typ"] = typ_dict[typ];
    bool is_relation = (RELATION_TYPS.count(typ_dict[typ]));

    // get sender/receiver if a relation
    if (is_relation) {
        nodeid = my_nodeid->second - num_nodes;
        if (metadata["typ"] == "used") {
            metadata["prov:entity"] = nodeid2id[nodeid >> id_bits];
            metadata["prov:activity"] = nodeid2id[nodeid & ((1 << id_bits) - 1)]; 
        }
        else if (metadata["typ"] == "wasGeneratedBy") {
            metadata["prov:activity"] = nodeid2id[nodeid >> id_bits];
            metadata["prov:entity"] = nodeid2id[nodeid & ((1 << id_bits) - 1)]; 
        }
        else if (metadata["typ"] == "wasDerivedFrom") {
            metadata["prov:usedEntity"] = nodeid2id[nodeid >> id_bits];
            metadata["prov:generatedEntity"] = nodeid2id[nodeid & ((1 << id_bits) - 1)]; 
        }
        else if (metadata["typ"] == "wasInformedBy") {
            metadata["prov:informant"]= nodeid2id[nodeid >> id_bits];
            metadata["prov:informed"] = nodeid2id[nodeid & ((1 << id_bits) - 1)]; 
        }
        else if (metadata["typ"] == "relation") {
            metadata["cf:sender"] = nodeid2id[nodeid >> id_bits];
            metadata["cf:receiver"] = nodeid2id[nodeid & ((1 << id_bits) - 1)]; 
        }
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
            // we can't decode because we don't know if this node is relative or not
            metadata[key_dict[key]] = "=";
        }
    }

    // decode encoded values
    for (size_t i = 0; i < num_encoded_keys; ++i) {
        metadata_bs->get_bits<unsigned char>(key, key_bits, cur_pos);
        cur_pos += key_bits;
        metadata_bs->get_bits<unsigned char>(encoded_val, val_bits, cur_pos);
        cur_pos += val_bits;
        metadata[key_dict[key]] = val_dict[encoded_val];
    }
    
    // decode common values
    for (size_t i = 0; i < num_common_keys; ++i) {
        metadata_bs->get_bits<unsigned char>(key, key_bits, cur_pos);
        cur_pos += key_bits;
        metadata_bs->get_bits<int>(common_val, COMMONSTR_BITS, cur_pos);
        cur_pos += COMMONSTR_BITS;
        metadata[key_dict[key]] = commonstr_dict[common_val];
    }

    // nonencoded values
    char label_key;
    for (size_t i = 0; i < num_other_keys; ++i) {
        metadata_bs->get_bits<unsigned char>(key, key_bits, cur_pos);
        cur_pos += key_bits;
        metadata_bs->get_bits<size_t>(val_size, MAX_STRING_SIZE_BITS, cur_pos);
        cur_pos += MAX_STRING_SIZE_BITS;
        metadata_bs->get_bits_as_str(str_val, val_size, cur_pos);
        cur_pos += val_size;

        if (key_dict[key] == "prov:label") {
            label_key = str_val[0];
            str_val = prov_label_dict[label_key] + str_val.substr(1) ;
        }
            
        metadata[key_dict[key]] = str_val; 
    }

    // get date
    map<int, int>date_diffs;
    for (size_t i = 0; i < num_diff_dates; ++i) {
        metadata_bs->get_bits<size_t>(date_index, DATE_TYPE_BITS, cur_pos);
        cur_pos += DATE_TYPE_BITS;
        metadata_bs->get_bits<int>(int_val, DATE_BITS[date_index], cur_pos);
        cur_pos += DATE_BITS[date_index];
        date_diffs[date_index] = int_val;
    }
    metadata["cf:date"] = "";
    for (size_t i = 0; i < default_date.size(); ++i) {
        if (i == 3) metadata["cf:date"] += "T";
        else if (i) metadata["cf:date"] += ":";

        auto diff_date = date_diffs.find(i);
        if (diff_date != date_diffs.end()) {
            metadata["cf:date"] += to_string(diff_date->second);
        } else {
            metadata["cf:date"] += to_string(default_date[i]);
        }
    }

    // we're done if this was a relation
    if (is_relation) {
        return metadata;
    }

    // else we need to encode equal keys in relation to another node or default
    auto relative = metadata.find(RELATIVE_NODE);
    map<string, string> relative_metadata;
    if (relative != metadata.end() && relative->second != "=" && stoi(relative->second) != (int)my_nodeid->second) {
        // the node was encoded in relation to another node
        relative_metadata = get_metadata(nodeid2id[stoi(relative->second)]);
    } else {
        // the node was encoded in relation to the default data
        relative_metadata = default_node_data; 
    }
    for (auto kv: metadata) {
        if (kv.second == "=") {
            metadata[kv.first] = relative_metadata[kv.first];
        }
    }
    return metadata;
}
Node_Id CompressedMetadata::get_node_id(string identifer) { return id2nodeid[identifer]; }
string CompressedMetadata::get_identifier(Node_Id node) { return nodeid2id[node]; }
vector<string> CompressedMetadata::get_node_ids() {
    vector<string> v(identifiers.begin(), identifiers.begin()+num_nodes);
    return v;
}

const string CompressedMetadata::PROV_DICTS_FILE = "../compression/prov_data_dicts.txt";
const string CompressedMetadata::IDENTIFIERS_FILE = "../compression/identifiers.txt";
const string CompressedMetadata::COMMONSTR_FILE = "../compression/common_strs";
const string CompressedMetadata::RELATIVE_NODE = "@";
const vector<size_t> CompressedMetadata::DATE_BITS = {12,4,5,5,6,6};
const size_t CompressedMetadata::DATE_TYPE_BITS = nbits_for_int(CompressedMetadata::DATE_BITS.size());
const size_t CompressedMetadata::COMMONSTR_BITS = nbits_for_int(CompressedMetadata::MAX_COMMON_STRS);
