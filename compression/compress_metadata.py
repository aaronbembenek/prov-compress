import json
import sys
import math
import bitstring
from process_json import (
    DICT_BEGIN,
    DICT_END,
    RECOGNIZED_TYPS,
)
import util
from collections import defaultdict

'''
TODO
    - Label stuff
    - Use byte encoding instead of bitstring
    - Compress numbers (flag with one bit)
'''

class Encoder():
    MAX_STRING_SIZE_BITS = 10 
    RELATIVE_NODE = '@'
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
    key_strings = {
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
    # (prov:label is the key, and have a value following)
    prov_label_strings = {
        '[address]', '[path]', '[TODO]', '[task]', '[unknown]', '[block special]', '[char special]', 
        '[directory]', '[fifo]', '[link]', '[file]', '[socket]', 
    }
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
    key_bits = util.nbits_for_int(len(key_strings))
    label_bits = 8 
    typ_bits = util.nbits_for_int(len(typ_strings))
    val_bits = max(util.nbits_for_int(len(val_strings)), util.nbits_for_int(len(node_types)))
    date_bits = {
        "year": 12,
        "month": 4,
        "day": 5,
        "hour": 5,
        "minute": 6,
        "sec": 6,
        "subsec": 10,
    }
    date_types = ["year","month","day","hour","minute","sec","subsec"]
    date_type_bits = util.nbits_for_int(len(date_types))

    def __init__(self, graph, metadata, iti):
        self.graph = graph
        self.metadata = metadata 
        self.iti = iti
        self.num_nodes = len(graph)
        self.id_bits = util.nbits_for_int(self.num_nodes)

        self.default_node_data = {}
        self.default_relation_data = {}
        self.default_time = []

        self.keys_dict = {elt:util.int2bitstr(i, Encoder.key_bits) for (i, elt) in enumerate(Encoder.key_strings)}
        self.vals_dict = {elt:util.int2bitstr(i, Encoder.val_bits) for (i, elt) in enumerate(Encoder.val_strings)}
        self.typs_dict = {elt:util.int2bitstr(i, Encoder.typ_bits) for (i, elt) in enumerate(Encoder.typ_strings)}
        self.node_types_dict = {elt:util.int2bitstr(i, Encoder.val_bits) 
                for (i, elt) in enumerate(Encoder.node_types)}

        self.labels_dict = {elt:util.int2bitstr(i, Encoder.label_bits) 
                for (i, elt) in enumerate(Encoder.prov_label_strings)}

        with open("prov_data_dicts.txt", 'w') as f:
            f.write(str(self.keys_dict))
            f.write(str(self.vals_dict))
            f.write(str(self.labels_dict))
            f.write(str(self.typs_dict))
            f.write(str(self.node_types_dict))

class CompressionEncoder(Encoder):
    def prepare_metadata_json(self):
        ''' 
            - reference encodes metadata JSON with corresponding defaults
            - creates and adds relation identifiers' IDS to dict
        '''
        id_dict = {}
      
        for identifier, metadata in self.metadata.items():
            relative_encoding = False
            default_data = self.default_node_data
            # since we can't modify the metadata in place during the loop, make a copy
            if metadata.typ == 'relation':
                default_data = self.default_relation_data
                # we don't want to include sender/receiver in metadata
                del metadata.data['cf:sender']
                del metadata.data['cf:receiver']
            elif 'cf:id' not in metadata.data:
                # this is some default camflow metadata
                continue
            else:
                cf_id = metadata.data['cf:id']
                if cf_id in id_dict and cf_id != None:
                    relative_encoding = True
                    # this id has had self.metadata defined for it before.
                    # compress with reference to this self.metadata
                    # Note that node metadata requires unpacking the relative metadata, then
                    # calculate the node's data in reference to that relative
                    default_data = id_dict[cf_id][1]
                else: 
                    default_data = self.default_node_data
                    # store the original metadata of the first time we see a camflow ID
                    id_dict[cf_id] = (identifier, metadata.data.copy())
            for key, val in metadata.data.items():
                # time is encoded off of a default time variable
                if key == "cf:date":
                    info = val.split('T')
                    date = val.split('T')[0].split(':')
                    time = val.split('T')[1].split(':')
                    year = int(date[0])
                    month = int(date[1])
                    day = int(date[2])
                    hour = int(time[0])
                    minute = int(time[1])
                    sec = int(time[2].split('.')[0])
                    subsec = int(time[2].split('.')[1])
                    time_info = [year, month, day, hour, minute, sec, subsec]
                    if len(self.default_time) == 0:
                        self.default_time = time_info.copy()
                    for i, t in enumerate(self.default_time):
                        if time_info[i] == t:
                            time_info[i] = 'same'
                    metadata.data[key] = time_info
                    continue
                # set the default node if there have been no nodes 
                # with this key compressed so far
                # only add the key if we're encoding w.r.t. the default rather than 
                # a previous node version
                elif key not in default_data and not relative_encoding:
                    assert(key != Encoder.RELATIVE_NODE)
                    default_data[key] = val
                # delta encode with reference to the default
                elif val == default_data[key]:
                    metadata.data[key] = 'same'
            
            # add a marker in the metadata that this is encoded relative 
            # to another node with the same cf_id
            if metadata.typ != 'relation' and cf_id in id_dict and cf_id != None:
                metadata.data[Encoder.RELATIVE_NODE] = self.iti[id_dict[cf_id][0]]
 
    def encode_metadata_entry(self, metadata):
        entry = ''
        equal_keys = []
        encoded_keys = []
        other_keys = []
        diffs = []
        encoded_date = ''
        for key, val in metadata.items():
            # encode relative date
            if key == 'cf:date':
                for i,t in enumerate(val):
                    if t != 'same':
                        diffs.append(util.int2bitstr(i, Encoder.date_type_bits)
                                    + util.int2bitstr(int(t), Encoder.date_bits[Encoder.date_types[i]]))
                encoded_date = ''.join(diffs)
                continue

            elif val == 'same':
                equal_keys.append(self.keys_dict[key])
                continue
            
            # ENCODED VALUES 
            # if val is a common string, replace it with an identifier
            if isinstance(val, str) and val in self.vals_dict:
                encoded_keys.append(self.keys_dict[key]+self.vals_dict[val])
            # replace the types
            elif key == 'cf:type':
                if isinstance(val, int):
                    encoded_keys.append(self.keys_dict[key]+self.node_types_dict[val])
            
            # this is some other key we couldn't encode 
            # record the length of the string in bits
            else:
                # replace any labels
                if key == 'prov:label':
                    for label in self.labels_dict:
                        val = val.replace(label, int(self.labels_dict[label], 2).to_bytes(1, 'big').decode())
                byts = (str(val)).encode('utf-8')
                bits = ''.join(["{0:08b}".format(x) for x in byts])
                other_keys.append(self.keys_dict[key]
                        +util.int2bitstr(len(bits), Encoder.MAX_STRING_SIZE_BITS)
                        +bits)

        entry += (util.int2bitstr(len(equal_keys), Encoder.key_bits) 
            + util.int2bitstr(len(encoded_keys), Encoder.key_bits) 
            + util.int2bitstr(len(other_keys), Encoder.key_bits) 
            + util.int2bitstr(len(diffs), Encoder.key_bits) 
            + ''.join(equal_keys) 
            + ''.join(encoded_keys) 
            + ''.join(other_keys)
            + encoded_date)
        return entry

    def encode_json(self):
        '''
        HEADER
            32 bits: total size of metadata
            default date (sum over all date types of date_bits[date_type])
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
            encoded date
                date_type_bits + date_bits[date_type] for all the parts of the time
                that differ from the default.
        '''
        # encode default data 
        default_data = ''
        for i, t in enumerate(self.default_time):
            default_data += util.int2bitstr(t, Encoder.date_bits[Encoder.date_types[i]])
        default_data += self.encode_metadata_entry(self.default_node_data)
        default_data += self.encode_metadata_entry(self.default_relation_data)

        # encode node data (in order of increasing rank)
        entry_data = ''
        sorted_idents = sorted(self.iti.keys(), key=lambda v: self.iti[v])
        for i, identifier in enumerate(sorted_idents): 
            if self.metadata[identifier].typ == 'relation':
                entry_data += util.int2bitstr(self.iti[identifier], 2*self.id_bits)
            entry_data += self.typs_dict[self.metadata[identifier].typ]
            entry_data += self.encode_metadata_entry(self.metadata[identifier].data)

        # add on 32-bit integer to represent the total number of bits sent
        s = util.int2bitstr(len(entry_data) + len(default_data) + 32, 32) + default_data + entry_data
        return s

    def compress_metadata(self, outfile):
        self.prepare_metadata_json()
        with open(outfile, 'wb') as f:
            bitstring.BitArray(bin=self.encode_json()).tofile(f)
