import json
import sys
import math
from process_json import (
    DICT_BEGIN,
    DICT_END,
    RELATIVE_NODE,
    VALUES_SEP,
    IDENTIFIER_SEP,
    KEY_VAL_SEP,
    UNKNOWN,
    RECOGNIZED_TYPS,
    Metadata,
    get_bits
)
from collections import defaultdict

'''
TODO
    - Convert JSON to BSON
    - Convert JSON to UBJSON
    - Compute common strings
    - Time compression?
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
    node_type_bits = math.ceil(math.log(len(node_types), 2))
    typ_strings = {
        'prefix', 'activity', 'relation', 'entity', 'agent', 'message', 
        'used', 'wasGeneratedBy', 'wasInformedBy', 'wasDerivedFrom',
        'unknown'
    }
    typ_bits = math.ceil(math.log(len(typ_strings), 2))
    key_strings = {
        DEFAULT_NODE_KEY, DEFAULT_RELATION_KEY,
        # cf: keys
        'cf:id', 'cf:boot_id', 'cf:machine_id', 'cf:date', 'cf:taint', 'cf:type', 'cf:version', 
        'cf:allowed', 'cf:sender', 'cf:receiver', 'cf:jiffies', 'cf:offset', 'cf:hasParent', 
        'cf:uid', 'cf:uuid', 'cf:gid', 'cf:pid', 'cf:vpid', 'cf:mode', 'cf:sock_type', 'cf:family', 
        'cf:seq', 'cf:protocol', 'cf:message', 'cf:address', 'cf:pathname', 
        # prov: keys
        'prov:label', 'prov:entity', 'prov:activity', 'prov:informant', 'prov:informed', 
        'prov:usedEntity', 'prov:generatedEntity', 'prov:type', 

    }
    key_bits = math.ceil(math.log(len(key_strings), 2))
    # (prov:label is the key, and have a value following)
    prov_label_strings = {
        '[address]', '[path]', '[TODO]', '[task]', '[unknown]', '[block special]', '[char special]', 
        '[directory]', '[fifo]', '[link]', '[file]', '[socket]', 
    }
    label_bits = math.ceil(math.log(len(prov_label_strings), 2))
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
    }
    val_bits = math.ceil(math.log(len(val_strings), 2))

    def __init__(self, graph, metadata, iti):
        self.graph = graph
        self.metadata = metadata 
        self.iti = iti
        self.keys_dict = {elt:get_bits(i, Encoder.key_bits) for (i, elt) in enumerate(Encoder.key_strings)}
        self.vals_dict = {elt:get_bits(i, Encoder.val_bits) for (i, elt) in enumerate(Encoder.val_strings)}
        self.labels_dict = {elt:get_bits(i, Encoder.label_bits) 
                for (i, elt) in enumerate(Encoder.prov_label_strings)}
        self.typs_dict = {elt:get_bits(i, Encoder.typ_bits) for (i, elt) in enumerate(Encoder.typ_strings)}
        self.node_types_dict = {elt:get_bits(i, Encoder.node_type_bits) 
                for (i, elt) in enumerate(Encoder.node_types)}

        with open("prov_data_dicts.txt", 'w') as f:
            f.write(str(self.keys_dict))
            f.write(str(self.vals_dict))
            f.write(str(self.labels_dict))
            f.write(str(self.typs_dict))
            f.write(str(self.node_types_dict))

    def encode_json(self):
        ''' 
        Encodes the JSON in place:
            - reference encodes metadata with corresponding defaults
            - replaces common strings with their identifiers
            - replaces identifiers with their mapped_to number/int ID
        return: dictionary in JSON format
        '''

        # Do one pass to replace common strings and the identifiers
        # since we can't modify the metadata in place during the loop, make a copy
        my_metadata = {}
        for identifier, metadata in self.metadata.items():
            new_metadata = {}
            for key, val in metadata.data.items():
                new_val = val
                new_key = key
                # replace the key
                if key in self.keys_dict:
                    new_key = self.keys_dict[key]
                # replace any labels
                if key == 'prov:label':
                    for label in self.labels_dict:
                        new_val = new_val.replace(label, self.labels_dict[label])
                # replace identifiers
                elif key == 'cf:sender' or key == 'cf:receiver':
                    new_val = self.iti[val]
                elif key == 'cf:type' and metadata.typ != 'relation' and val != None:
                    new_val = self.node_types_dict[val]
                # replace any other strings in the value (must be full match)
                if isinstance(val, str) and val in self.vals_dict:
                    new_val = self.vals_dict[val]
                # set metadata.data appropriately
                new_metadata[new_key] = new_val
           
            # replace identifier with ID key, metadata with the encoded metadata
            my_metadata[self.iti[identifier]] = Metadata(self.typs_dict[metadata.typ], new_metadata)
        self.metadata = my_metadata

        # Do another pass to encode with reference to default metadata
        default_node_data = {}
        default_relation_data = {}
        id_dict = {}
        
        for intid, metadata in self.metadata.items():
            default_data = default_node_data
            # since we can't modify the metadata in place during the loop, make a copy
            if metadata.typ == self.typs_dict['relation']:
                default_data = default_relation_data
            else:
                cf_id = metadata.data[self.keys_dict['cf:id']]
                if cf_id in id_dict and cf_id != None:
                    # this id has had self.metadata defined for it before.
                    # compress with reference to this self.metadata
                    # Note that node metadata requires unpacking the relative metadata, then
                    # calculate the node's data in reference to that relative
                    default_data = id_dict[cf_id][1]
                else: 
                    default_data = default_node_data
                    # store the original metadata of the first time we see a camflow ID
                    id_dict[cf_id] = (intid, metadata.data)
            for encoded_key, val in metadata.data.items():
                # set the default node if there have been no nodes 
                # with this key compressed so far
                if encoded_key not in default_data:
                    assert(encoded_key != RELATIVE_NODE)
                    default_data[encoded_key] = val
                # delta encode with reference to the default
                if val == default_data[encoded_key]:
                    metadata.data[encoded_key] = 0 
            
            # add a marker in the metadata that this is encoded relative 
            # to another node with the same cf_id
            if metadata.typ != self.typs_dict['relation'] and cf_id in id_dict and cf_id != None:
                metadata.data[RELATIVE_NODE] = cf_id 
        
        # add defaults to the metadata dictionary under "dnode" and "dedge" keys
        self.metadata[Encoder.DEFAULT_NODE_KEY] = default_node_data
        self.metadata[Encoder.DEFAULT_RELATION_KEY] = default_relation_data
        return self.metadata

    def compress_metadata(self):
        raise NotImplementedError()

    def json_to_str(self):
        s = ''
        for intid, metadata in self.metadata:
            s.append(intid)
            metadata.typ
            for key, val in metadata.data.items():
                s += key + val

# translates JSON into a string with delimiters
class StringEncoder(Encoder):
    # Returns a string representing the compressed node self.metadata for the self.graph.
    def compress_node_metadata(self):
        '''
        Example output:
            The first line is not part of the table: it is the default reference string
            ,-separated input are the data fields for the node keys (defined below)
            $: marks an extra keys/data pair
            #: marks a new node
            ?: marks undefined identifiers (with no self.metadata)
            a@b: marks when self.metadata is reference encoded off of the self.metadata of a previous identifier with the same ID
                a is the integer corresponding to the identifier of this node
                b is the index of the node in the list (i.e. the number of #s) of 
                    the first instance of another identifier with the same ID as (a)

        None,457525,4,1516773907,734854839,9,2016:11:03T22:07:06.978,[]#
        2n,0,0,0,0,0,0,0,cf:gid$1000,cf:uid$1000,prov:label$[task] 9#
        4n,?#
        42n,?#
        11n,5,0,0,0,1,2016:11:03T22:07:09.119,0,cf:gid$1000,cf:uid$1000,prov:label$[task] 10#
        35n,?#
        19n@4n,0,0,0,0,2,0,0,0,0,prov:label$[task] 11#
        '''
        node_keys = {
                'cf:id':1,
                'cf:type':2,
                'cf:boot_id':3,
                'cf:machine_id':4,
                'cf:version':5,
                'cf:date':6,
                'cf:taint':7,
        }

        id_dict = {}
        compressed_nodes = []
        default_node_data = [None for _ in range(len(node_keys)+1)]
        have_set_default = False
        for identifier, data in self.metadata.items():
            if data.typ == "relation":
                continue
            if data.data['cf:type'] == None:
                node_data = [str(self.iti[identifier]), UNKNOWN]
            else:
                node_data = [None for _ in range(len(node_keys)+1)]

                # encode the data with reference to the default string
                node_data[0] = self.iti[identifier]
                for key, datum in data.data.items():
                    val = datum 
                    # this key is not in the default self.metadata for nodes
                    if key not in node_keys:
                        node_data.append(str(key) + KEY_VAL_SEP + str(val))
                    else:
                        # set the default node if there have been no nodes compressed so far
                        if not have_set_default:
                            default_node_data[node_keys[key]] = val
                        # delta encode with reference to the default node data
                        if val == default_node_data[node_keys[key]]:
                            node_data[node_keys[key]] = 0 
                        # for now, let's not delta encode the strings (including the time)
                        elif isinstance(val, str):
                            node_data[node_keys[key]] = val
                        else:
                            assert(isinstance(val, int))
                            node_data[node_keys[key]] = val - default_node_data[node_keys[key]]

                node_data = list(map(str,node_data))

                # see if we can compress node_data further by referencing another node 
                # with the same camflow ID
                cf_id = data.data['cf:id']
                # store the index of the first time we see this ID and the corresponding self.metadata
                if cf_id not in id_dict:
                    id_dict[cf_id] = (self.iti[identifier], node_data)
                else:
                    # this id has had self.metadata defined for it before. compress with reference to this self.metadata
                    if cf_id in id_dict:
                        id_data = id_dict[cf_id][1]
                        node_data[0] = str(node_data[0]) +RELATIVE_NODE+str(id_dict[cf_id][0])
                        for i, val in enumerate(node_data):
                            # delta encode with reference to the id node data
                            # for right now, we just see if the data was equivalent
                            if val == id_data[i]:
                                node_data[i] = '0'

                if not have_set_default:
                    have_set_default = True
            compressed_nodes.append(node_data)
        
        default_node_data = list(map(str, default_node_data))
        s = [VALUES_SEP.join(default_node_data)]
        for ls in compressed_nodes:
            s.append(','.join(ls))
        s = IDENTIFIER_SEP.join(s)
        s = DICT_BEGIN + s + DICT_END
        return s

    # Returns a string representing the compressed relation self.metadata for the self.graph.
    # Currently, only integer references are compressed (delta-encoded against the default)
    # Strings (including time) are written without compression
    def compress_relation_metadata(self):
        '''
        Example output:
            The first line is not part of the table: it is the default reference string
            ,-separated input are the data fields for the relation keys (defined below)
            $: marks an extra keys/data pair
            #: marks a new relation

            160r,1516773907,734854839,None,2016:11:03T22:07:09.119,[],open,open,true,36,45#
            0r,0,0,None,0,0,0,0,0,0,0#
            2r,0,0,None,0,0,read,read,0,0,-44#
            -21r,0,0,None,2016:11:03T22:07:06.978,0,version,version,0,-30,-16
        '''
        # common fields of all relations
        # offset is optional
        relation_keys = {
            'cf:id':1,
            'cf:boot_id':2,
            'cf:machine_id':3,
            'cf:date':4,
            'cf:taint':5,
            'cf:type':6,
            'prov:label':7,
            'cf:allowed':8,
            'cf:sender':9,
            'cf:receiver':10
        }
        compressed_relations = []
        default_relation_data = [None for _ in range(len(relation_keys)+1)]
        have_set_default = False
        for identifier, data in self.metadata.items():
            if data.typ != "relation":
                continue
            
            # using lists to ensure that the keys and relation data are in the same order
            relation_data = [None for _ in range(len(relation_keys)+1)]
            relation_data[0] = self.iti[identifier]
            for key, datum in data.data.items():
                val = datum 
                # this key is not in the default self.metadata for relations
                if key not in relation_keys:
                    relation_data.append(str(key) + KEY_VAL_SEP + str(val))
                else:
                    # we want to not encode the entire identifier---just map to ints
                    if key == 'cf:sender' or key == 'cf:receiver':
                        val = self.iti[datum]
                    # set the default relation if there have been no relations compressed so far
                    if not have_set_default:
                        default_relation_data[relation_keys[key]] = val
                    # delta encode with reference to the default relation data
                    if val == default_relation_data[relation_keys[key]]:
                        relation_data[relation_keys[key]] = 0 
                    # for now, let's not delta encode the strings (including the time)
                    elif isinstance(val, str):
                        relation_data[relation_keys[key]] = val
                    else:
                        assert(isinstance(val, int))
                        relation_data[relation_keys[key]] = val - default_relation_data[relation_keys[key]]
            if not have_set_default:
                have_set_default = True

            # everything has to be strings...
            relation_data = list(map(str,relation_data))
            compressed_relations.append(relation_data)
        default_relation_data = list(map(str, default_relation_data))

        # join all strings together
        s = [VALUES_SEP.join(default_relation_data)]
        for ls in compressed_relations:
            s.append(','.join(ls))
        s = IDENTIFIER_SEP.join(s)
        s = DICT_BEGIN + s + DICT_END
        return s


    # Write a dict mapping identifiers to ints, compressed relation self.metadata, and compressed
    # node self.metadata to the provide outfile.
    # Currently, only integer references are compressed (delta-encoded against the default)
    # Strings (including time) are written without compression
    def compress_metadata(self):
        s = DICT_BEGIN 
        for k, v in self.iti.items():
            s += k + KEY_VAL_SEP + str(v) + IDENTIFIER_SEP
        s += DICT_END
        iti_str = s.replace(' ', '').replace("'",'')
        cnm = self.compress_node_metadata() 
        crm = self.compress_relation_metadata() 
        return iti_str+cnm+crm
