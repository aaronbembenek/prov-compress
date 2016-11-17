import json
import sys
import math
import bitstring
from process_json import (
    DICT_BEGIN,
    DICT_END,
    MAX_STRING_SIZE_BITS,
    RELATIVE_NODE,
    RECOGNIZED_TYPS,
    Metadata,
    get_bits,
)
import util
from collections import defaultdict

'''
TODO
    - Time compression?
    - Use byte encoding instead of bitstring
'''

class Encoder():
    DEFAULT_NODE_KEY = '-1'
    DEFAULT_RELATION_KEY = '-2'
    node_types = {
        0x00000001, 0x00000002, 0x00000004, 0x00000008, 0x00000010, 
        0x00000020, 0x00000040, 0x00000080, 0x00000100, 0x00000200, 
        0x00000400, 0x00000800, 0x00001000, 0x00002000, 0x00004000, 
        0x00008000, 0x00010000, 0x00020000, 0x00040000, 0x00080000, 
        0x00100000, 0x00200000,
    }    
    typ_strings = {
        'prefix', 'activity', 'relation', 'entity', 'agent', 'message', 
        'used', 'wasGeneratedBy', 'wasInformedBy', 'wasDerivedFrom',
        'unknown'
    }
    typ_bits = util.nbits_for_int(len(typ_strings))
    key_strings = {
        DEFAULT_NODE_KEY, DEFAULT_RELATION_KEY,
        # cf: keys
        'cf:id', 'cf:boot_id', 'cf:machine_id', 'cf:date', 'cf:taint', 'cf:type', 'cf:version', 
        'cf:allowed', 'cf:sender', 'cf:receiver', 'cf:jiffies', 'cf:offset', 'cf:hasParent', 
        'cf:uid', 'cf:uuid', 'cf:gid', 'cf:pid', 'cf:vpid', 'cf:mode', 'cf:sock_type', 'cf:family', 
        'cf:seq', 'cf:protocol', 'cf:message', 'cf:address', 'cf:pathname', 
        'cf:camflow', 'cf:machine', 'cf:sysname', 'cf:nodename', 'cf:release',
        # prov: keys
        'prov:label', 'prov:entity', 'prov:activity', 'prov:informant', 'prov:informed', 
        'prov:usedEntity', 'prov:generatedEntity', 'prov:type', 
        # additional keys for relative versions
        RELATIVE_NODE
    }
    key_bits = util.nbits_for_int(len(key_strings))
    # (prov:label is the key, and have a value following)
    prov_label_strings = {
        '[address]', '[path]', '[TODO]', '[task]', '[unknown]', '[block special]', '[char special]', 
        '[directory]', '[fifo]', '[link]', '[file]', '[socket]', 
    }
    label_bits = util.nbits_for_int(len(prov_label_strings))
    val_strings = {
        # Booleans
        'false', 'true',
        # Edge labels
        'unknown', 'read', 'write', 'create', 'pass', 'change', 'mmap_write', 'attach', 'associate', 
        'bind', 'connect', 'listen', 'accept', 'open', 'parent', 'version', 'link', 
        'named', 'ifc', 'exec', 'clone', 'version', 'search', 'mmap_read', 'mmap_exec', 
        'send', 'receive', 'perm_read', 'perm_write', 'perm_exec',
        # Node types
        'string', 'relation', 'task', 'inode_unknown', 'link', 'file', 'directory', 
        'char', 'block', 'fifo', 'socket', 'msg', 'shm', 'sock', 'address', 'sb', 'file_name', 
        'ifc', 'disc_entity', 'disc_activity', 'disc_agent', 'disc_node', 'packet', 'mmaped_file',
        # For prov
        'cf:file'
    }
    val_bits = max(util.nbits_for_int(len(val_strings)), util.nbits_for_int(len(node_types)))

    def __init__(self, graph, metadata, iti):
        self.graph = graph
        self.metadata = metadata 
        self.iti = iti
        self.num_nodes = len(iti)
        self.id_bits = util.nbits_for_int(len(iti))

        self.keys_dict = {elt:get_bits(i, Encoder.key_bits) for (i, elt) in enumerate(Encoder.key_strings)}
        self.vals_dict = {elt:get_bits(i, Encoder.val_bits) for (i, elt) in enumerate(Encoder.val_strings)}
        self.labels_dict = {elt:get_bits(i, Encoder.label_bits) 
                for (i, elt) in enumerate(Encoder.prov_label_strings)}
        self.typs_dict = {elt:get_bits(i, Encoder.typ_bits) for (i, elt) in enumerate(Encoder.typ_strings)}
        self.node_types_dict = {elt:get_bits(i, Encoder.val_bits) 
                for (i, elt) in enumerate(Encoder.node_types)}

        with open("prov_data_dicts.txt", 'w') as f:
            f.write(str(self.keys_dict))
            f.write(str(self.vals_dict))
            f.write(str(self.labels_dict))
            f.write(str(self.typs_dict))
            f.write(str(self.node_types_dict))


    def prepare_metadata_json(self):
        ''' 
            - reference encodes metadata JSON with corresponding defaults
            - creates and adds relation identifiers' IDS to dict
            - writes sorted list of identifiers to file
        '''
        default_node_data = {}
        default_relation_data = {}
        id_dict = {}
        num_nodes = len(self.iti)
      
        for identifier, metadata in self.metadata.items():
            default_data = default_node_data
            # since we can't modify the metadata in place during the loop, make a copy
            if metadata.typ == 'relation':
                default_data = default_relation_data
                # we don't want to include sender/receiver in metadata
                del metadata.data['cf:sender']
                del metadata.data['cf:receiver']
            else:
                if 'cf:id' not in metadata.data:
                    # this is some default camflow metadata
                    continue
                else:
                    cf_id = metadata.data['cf:id']
                    if cf_id in id_dict and cf_id != None:
                        # this id has had self.metadata defined for it before.
                        # compress with reference to this self.metadata
                        # Note that node metadata requires unpacking the relative metadata, then
                        # calculate the node's data in reference to that relative
                        default_data = id_dict[cf_id]
                    else: 
                        default_data = default_node_data
                        # store the original metadata of the first time we see a camflow ID
                        id_dict[cf_id] = metadata.data
            for key, val in metadata.data.items():
                # set the default node if there have been no nodes 
                # with this key compressed so far
                if key not in default_data:
                    assert(key != RELATIVE_NODE)
                    default_data[key] = val
                # delta encode with reference to the default
                if val == default_data[key]:
                    metadata.data[key] = 'same'
            
            # add a marker in the metadata that this is encoded relative 
            # to another node with the same cf_id
            if metadata.typ != 'relation' and cf_id in id_dict and cf_id != None:
                metadata.data[RELATIVE_NODE] = cf_id 
        
        # add defaults to the metadata dictionary under default 
        self.metadata[Encoder.DEFAULT_NODE_KEY] = default_node_data
        self.metadata[Encoder.DEFAULT_RELATION_KEY] = default_relation_data
 
    def encode_metadata_entry(self, metadata):
        entry = ''
        equal_keys = []
        encoded_keys = []
        other_keys = []
        for key, val in metadata.items():
            if val == 'same':
                equal_keys.append(self.keys_dict[key])
            
            # encode the value here
            # replace any labels
            if key == 'prov:label':
                for label in self.labels_dict:
                    val = val.replace(label, self.labels_dict[label])
            # replace any common strings
            elif isinstance(val, str) and val in self.vals_dict:
                encoded_keys.append(self.keys_dict[key]+self.vals_dict[val])
            # replace the types
            elif key == 'cf:type' and isinstance(val, int):
                encoded_keys.append(self.keys_dict[key]+self.node_types_dict[val])
            
            # this is some other key we couldn't encode 
            # record the length of the string in bits
            else:
                byts = (str(val)).encode('utf-8')
                bits = ''.join(["{0:08b}".format(x) for x in byts])
                other_keys.append(self.keys_dict[key]+get_bits(len(bits), MAX_STRING_SIZE_BITS)+bits)

        entry += (get_bits(len(equal_keys), Encoder.key_bits) 
            + get_bits(len(encoded_keys), Encoder.key_bits) 
            + get_bits(len(other_keys), Encoder.key_bits) 
            + ''.join(equal_keys) 
            + ''.join(encoded_keys) 
            + ''.join(other_keys))
        return entry

    def encode_json(self):
        '''
        HEADER
            32 bits: total size of metadata
            node default data (see below: typ/ID will not be included)
            relation default data (see below: typID will not be included)

        An entry for an identifier looks like
            2*id_bits   [ID (RELATION ONLY)]
            typ_bits    [type]
            key_bits    [num_equal_keys]
            key_bits    [num_encoded_keys]
            key_bits    [num_other_keys]
            key_bits*num_equal_keys                 [listof(equal_keys)]
            (key_bits+val_bits)*num_encoded_keys    [listof(key+encoded_value)]
            (key_bits+8+value_len)*num_other_keys   [listof(key+val_length(bytes)+value)]
        '''
        # encode default data 
        default_data = ''
        for defaults in [self.metadata[Encoder.DEFAULT_NODE_KEY], self.metadata[Encoder.DEFAULT_RELATION_KEY]]: 
            default_data += self.encode_metadata_entry(defaults)
        del self.metadata[Encoder.DEFAULT_NODE_KEY]
        del self.metadata[Encoder.DEFAULT_RELATION_KEY]

        # encode node data (in order of increasing rank)
        entry_data = ''
        sorted_idents = sorted(self.iti.keys(), key=lambda v: self.iti[v])
        for identifier in sorted_idents: 
            if self.metadata[identifier].typ == 'relation':
                entry_data += get_bits(self.iti[identifier], 2*self.id_bits)
            entry_data += self.typs_dict[self.metadata[identifier].typ]
            entry_data += self.encode_metadata_entry(self.metadata[identifier].data)

        # 32-bit integer to represent the total number of bits sent
        s = get_bits(len(entry_data), 32) + default_data + entry_data
        return s

    def compress_metadata(self):
        self.prepare_metadata_json()
        with open('compressed_metadata.txt', 'wb') as f:
            bitstring.BitArray(bin=self.encode_json()).tofile(f)
