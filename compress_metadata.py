import json
import sys
from process_json import (
    DICT_BEGIN,
    DICT_END,
    RELATIVE_NODE,
    VALUES_SEP,
    IDENTIFIER_SEP,
    KEY_VAL_SEP,
    UNKNOWN,
    RECOGNIZED_TYPS
)
from collections import defaultdict

'''
TODO:
    - Construct common strings dictionary 
    - Do reference encoding/string-id replacement within JSON dictionary
    - Convert JSON to string
    - Convert JSON to BSON
    - Convert JSON to UBJSON
'''

class Encoder():
    common_strings = {
        # cf: keys
        'cf:id', 'cf:boot_id', 'cf:machine_id', 'cf:date', 'cf:taint', 'cf:type', 'cf:version', 
        'cf:allowed', 'cf:sender', 'cf:receiver', 'cf:jiffies', 'cf:offset', 'cf:hasParent', 
        'cf:uid', 'cf:uuid', 'cf:gid', 'cf:pid', 'cf:vpid', 'cf:mode', 'cf:sock_type', 'cf:family', 
        'cf:seq', 'cf:protocol', 'cf:message', 'cf:address', 'cf:pathname', 
        # prov: keys
        'prov:label', 'prov:entity', 'prov:activity', 'prov:informant', 'prov:informed', 
        'prov:usedEntity', 'prov:generatedEntity', 'prov:type', 
        # Booleans
        'false', 'true',
        # Lables
        'address', 'path', 'TODO', 'task', 'unknown', 'block special', 'char special', 
        'directory', 'fifo', 'link', 'file', 'socket', 
        # Edge labels
        'read', 'write', 'create', 'pass', 'change', 'mmap_write', 'attach', 'associate', 
        'bind', 'connect', 'listen', 'accept', 'open', 'parent', 'version', 'link', 
        'named', 'ifc', 'exec', 'clone', 'version', 'search', 'mmap_read', 'mmap_exec', 
        'send', 'receive', 'perm_read', 'perm_write', 'perm_exec',
        # Node types
        'string', 'relation', 'task', 'inode_unknown', 'link', 'file', 'directory', 
        'char', 'block', 'fifo', 'socket', 'msg', 'shm', 'sock', 'address', 'sb', 'file_name', 
        'ifc', 'disc_entity', 'disc_activity', 'disc_agent', 'disc_node', 'packet', 'mmaped_file',
    }
    def __init__(self, graph, metadata, iti):
        self.graph = graph
        self.metadata = metadata 
        self.iti = iti
        self.common_strings_dict = {elt:i for (i, elt) in enumerate(Encoder.common_strings)}

    def encode_json(self):
        ''' 
        Encodes the JSON in place:
            - reference encodes metadata with corresponding defaults
            - replaces common strings with their identifiers
            - compress time format (also as reference encoded?)
        return: dictionary in JSON format
        '''

    def compress_node_metadata(self):
        raise NotImplementedError()

    def compress_edge_metadata(self):
        raise NotImplementedError()
    
    def compress_metadata(self):
        raise NotImplementedError()

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
