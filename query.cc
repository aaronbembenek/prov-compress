#include "query.hh"

void construct_prov_dicts() {
    ifstream infile;
    string buffer;
    vector<string> data;

    infile.open(PROV_DICTS_FILE, ios::in);
    assert(infile.is_open());
    getline(infile, buffer);
    infile.close();

    data = split(buffer, DICT_END);
    assert(data.size() == 5);

    std::vector<map<unsigned char, string>*> dicts = {&key_dict, &val_dict, &prov_label_dict, &typ_dict, &node_types_dict};
    std::vector<int*> bits = {&key_bits, &val_bits, &label_bits, &typ_bits, &node_type_bits};
    for (size_t i = 0; i < data.size(); ++i) {
        set_dict_entries(*dicts[i], data[i], 2);
        *bits[i] = nbits_for_int(dicts[i]->size());
    }
}

void construct_identifiers_dict() {
    ifstream infile;
    string buffer;

    infile.open(IDENTIFIERS_FILE, ios::in);
    assert(infile.is_open());
    getline(infile, buffer);
    infile.close();

    set_dict_entries(intid_to_id_dict, buffer, 10);
    // create the dictionary mapping the other direction
    for (auto i = intid_to_id_dict.begin(); i != intid_to_id_dict.end(); ++i) {
        id_to_intid_dict[i->second] = i->first; 
    }
}

void construct_metadata_dict(string& buffer) {
    BitSet bs(buffer); 
    size_t total_size, cur_pos;
   
    // get the total size of the data
    cur_pos = 0;
    bs.get_bits<size_t>(total_size, 32, cur_pos);
    cur_pos += 32;

    // get defaults
    vector<map<unsigned char, string>*> default_data_dicts = {&default_node_data, &default_relation_data};
    size_t num_equal_keys, num_encoded_keys, num_other_keys = 0;
    vector<size_t*> num_key_types = {&num_equal_keys, &num_encoded_keys, &num_other_keys};
    for (map<unsigned char, string>* default_data: default_data_dicts) {
        for (size_t* key : num_key_types) {
            bs.get_bits<size_t>(*key, key_bits, cur_pos);
            cur_pos += key_bits;
        }

        assert(num_equal_keys == 0);

        unsigned char key, val;
        for (size_t i = 0; i < num_encoded_keys; ++i) {
            bs.get_bits<unsigned char>(key, key_bits, cur_pos);
            cur_pos += key_bits;
            
            // we encode the cf:type as an integer if it is a node
            if (key_dict[key] == "cf:type" && (default_data != &default_relation_data)) {
                bs.get_bits<unsigned char>(val, node_type_bits, cur_pos);
                cur_pos += node_type_bits;
                (*default_data)[key] = node_types_dict[val];
            } else {
                bs.get_bits<unsigned char>(val, val_bits, cur_pos);
                cur_pos += val_bits;
                (*default_data)[key] = val_dict[val];
            }
        }

        string val_str;
        size_t val_size;
        for (size_t i = 0; i < num_other_keys; ++i) {
            bs.get_bits<unsigned char>(key, key_bits, cur_pos);
            cur_pos += key_bits;
            
            bs.get_bits<size_t>(val_size, MAX_STRING_SIZE_BITS, cur_pos);
            cur_pos += MAX_STRING_SIZE_BITS;
            
            bs.get_bits_as_str(val_str, val_size, cur_pos);
            cur_pos += val_size;
            (*default_data)[key] = val_str;
        }
    }

    // go through the rest of the string, creating a map from intid to string of bits
    while(1) {
        return;
    }
    //assert(cur_pos == total_size);
}

string decode_from_default(string& identifier, vector<string>& data) {
    (void)data;
    return identifier;
}

string get_metadata(string identifier) {
    return identifier;
}

int main(int argc, char *argv[]) {
    ifstream infile;
    string buffer;
    vector<string> data;

    if (argc == 1) {
        infile.open("compressed_metadata.txt", ios::in);
    } else if (argc == 2) {
        infile.open(argv[1], ios::in);
    } else {
        cout << "Usage: ./query [infile]" << endl;
        return 1;
    }
    if (!infile.is_open()) {
        cout << "Failed to open input file" << endl;
        return 1;
    }
    getline(infile, buffer);
    infile.close();

    construct_prov_dicts();
    construct_identifiers_dict();
    construct_metadata_dict(buffer);
    return 0;
}
