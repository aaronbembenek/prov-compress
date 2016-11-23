#include "metadata.hh"
#include "json/json.h"

/* DUMMY IMPLEMENTATION */
DummyMetadata::DummyMetadata(string& infile) {
    Json::Reader reader;
    Json::FastWriter fastWriter;
    Json::Value root;
    Json::Value json_typ;
    size_t pos;

    ifstream input(infile);
    for (string line; getline(input, line);) {
        string json;
        if ((pos = line.find(DICT_BEGIN)) == string::npos) {
            continue;
        }
        json = line.substr(pos);
        bool parsingSuccessful = reader.parse( json, root );
        if ( !parsingSuccessful ) {
            // report to the user the failure and their locations in the document.
            std::cout  << "Failed to parse configuration\n"
                       << reader.getFormattedErrorMessages();
            return;
        }
        for (auto typ : typs) {
            if (typ == "prefix") {
                continue;
            }
            json_typ = root.get(typ, "None");
            if (json_typ != "None") {
                auto ids = json_typ.getMemberNames();
                for (auto id : ids) {
                    json_typ[id]["typ"] = typ;
                    id2jsonstr[id] = fastWriter.write(json_typ[id]);
                }
            }
        }

    }
};

map<string, string> DummyMetadata::get_metadata(string& identifier) {
    map<string, string> m;
    Json::Reader reader;
    Json::FastWriter fastWriter;
    Json::Value root;

    if(id2jsonstr.count(identifier) == 0) {
        return m;
    }

    bool parsingSuccessful = reader.parse( id2jsonstr[identifier], root );
    if ( !parsingSuccessful ) {
        // report to the user the failure and their locations in the document.
        std::cout  << "Failed to parse configuration\n"
                   << reader.getFormattedErrorMessages();
        return m;
    }
    auto keys = root.getMemberNames();
    for (auto k: keys) {
        m[k] = fastWriter.write(root[k]);
    }
    return m;
}
vector<string> DummyMetadata::typs = {"prefix", "activity", "entity", "relation", "unknown"};

/*************************************
 * COMPRESSED IMPLEMENTATION 
 *************************************/
CompressedMetadata::CompressedMetadata(string& infile) {
    construct_identifiers_dict();
    construct_prov_dicts();
    construct_metadata_dict(infile);
}

void CompressedMetadata::construct_identifiers_dict() {
    string buffer, default_id, id;
    size_t default_len, cur_pos;
    vector<string> encoded_ids;

    read_file(IDENTIFIERS_FILE, buffer);

    BitSet bs(buffer);
    bs.get_bits(num_nodes, 32, 0);
    cur_pos = 32;
    id_bits = nbits_for_int(num_nodes);

    bs.get_bits(default_len, 32, cur_pos);
    cur_pos += 32;

    bs.get_bits_as_str(default_id, default_len*8, cur_pos);
    cur_pos += default_len*8;

    size_t offset, num_diffs, id_index = 0;
    char ch;
    while(cur_pos < buffer.length()*8) {
        id = default_id;
        bs.get_bits<size_t>(num_diffs, 8, cur_pos);
        cur_pos += 8;
        for (size_t i = 0; i < num_diffs; ++i) {
            bs.get_bits<size_t>(offset, 8, cur_pos);
            cur_pos += 8;
            bs.get_bits<char>(ch, 8, cur_pos);
            cur_pos += 8;
            id[offset] = ch;
        }
        if (id_index < num_nodes) {
            // not yet set for relations
            intid2id[id_index] = id; 
            id2intid[id] = id_index; 
        }
        identifiers.push_back(id);
        id_index++;
    }
}

void CompressedMetadata::construct_prov_dicts() {
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

size_t CompressedMetadata::find_next_entry(size_t cur_pos) {
    size_t num_equal_keys, num_encoded_keys, num_other_keys, num_diff_dates;
    vector<size_t*> num_key_types = {&num_equal_keys, &num_encoded_keys, &num_other_keys, &num_diff_dates};

    size_t date_index, val_size;
    cur_pos += typ_bits;
    for (size_t* key : num_key_types) {
        metadata_bs->get_bits<size_t>(*key, key_bits, cur_pos);
        cur_pos += key_bits;
    }
    cur_pos += key_bits*num_equal_keys;
    cur_pos += (key_bits+val_bits)*num_encoded_keys;
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
    string str_val;
    int int_val;

    size_t num_equal_keys, num_encoded_keys, num_other_keys, num_diff_dates;
    vector<size_t*> num_key_types = {&num_equal_keys, &num_encoded_keys, &num_other_keys, &num_diff_dates};

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
        cur_pos = find_next_entry(cur_pos);
    }
    // go through all relation data, creating a map from intid to index of string 
    int relation_id;
    int num_relations = 0;
    while(cur_pos < total_size) {
        metadata_bs->get_bits<int>(relation_id, 2*id_bits, cur_pos);
        cur_pos += 2*id_bits;

        intid2dataindex[relation_id] = cur_pos;

        cur_pos = find_next_entry(cur_pos);

        id2intid[identifiers[num_nodes + num_relations]] = relation_id;
        intid2id[relation_id] = identifiers[num_nodes + num_relations];
        num_relations++;
    }
    assert(cur_pos == total_size);
}

map<string, string> CompressedMetadata::get_metadata(string& identifier) {
    map<string, string> metadata;
    size_t cur_pos, val_size, date_index;
    unsigned char key, encoded_val, typ;
    int int_val;
    string str_val;

    size_t num_equal_keys, num_encoded_keys, num_other_keys, num_diff_dates;
    vector<size_t*> num_key_types = {&num_equal_keys, &num_encoded_keys, &num_other_keys, &num_diff_dates};

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

const string CompressedMetadata::PROV_DICTS_FILE = "../compression/prov_data_dicts.txt";
const string CompressedMetadata::IDENTIFIERS_FILE = "../compression/identifiers.txt";
const string CompressedMetadata::RELATIVE_NODE = "@";
const vector <size_t> CompressedMetadata::DATE_BITS = {12,4,5,5,6,6,10};
const size_t CompressedMetadata::DATE_TYPE_BITS = nbits_for_int(CompressedMetadata::DATE_BITS.size());
