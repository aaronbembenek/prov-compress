#!/usr/bin/python3

'''
Given a JSON file with Camflow-style provenance data, constructs a provenance
graph and a dictionary of metadata.
Provides functions to output data in different formats for dot/gspan processing.
'''

import json
import sys
from collections import defaultdict

RECOGNIZED_TYPS = ["prefix", "activity", "entity", "relation", "unknown"]

class Edge:
    def __init__(self, dest, label):
        self.dest = dest
        self.label = label

    def __repr__(self):
        return '(dest = "%s", label = "%s")' % (self.dest, self.label)

class Metadata:
    def __init__(self, typ, data):
        self.typ = typ
        self.data = data

    def __repr__(self):
        return '(typ = "%s", data = "%s")' % (self.typ, self.data)

# Returns an adjacency-list style graph and dictionary of metadata, both
# indexed by Camflow provenance identifier.
def process_json(infile):
    metadata = {}
    graph = {}
    missing_nodes = set()
    with open(infile) as f:
        for line in f:
            i = line.find("{")
            if i == -1:
                continue
            line = line[i:]
            for typ, entries in json.loads(line).items():
                assert typ in RECOGNIZED_TYPS and typ != "unknown"
                if typ == "prefix":
                    continue
                for identifier, data in entries.items():
                    # confirm that each entry in the JSON has a unique ID.
                    assert identifier not in metadata
                    metadata[identifier] = Metadata(typ, data)
                    missing_nodes.discard(identifier)
                    if typ == "relation":
                        tail = data["cf:receiver"]
                        head = data["cf:sender"]
                        # there are relations defined where the sender ID 
                        # is never actually defined as an entity/activity
                        # add empty metadata for these
                        for v in [head, tail]:
                            if v not in metadata:
                                missing_nodes.add(v)
                        graph.setdefault(tail, []).append(
                                Edge(head, identifier))
                    else:
                        # This is just so that we have a node in the graph
                        # for every entity/activity.
                        graph.setdefault(identifier, [])
    for i, identifier in enumerate(missing_nodes):
        assert identifier not in metadata
        metadata[identifier] = Metadata("unknown", {'cf:id':-i, 'cf:type':None})
        graph.setdefault(identifier, [])
    return graph, metadata

# Returns a dictionary mapping all identifier strings for vertices
# and edges in the graph to unique integers
def identifier_to_int(graph):
    iti = {}
    v_ctr = 0
    e_ctr = 0
    for v, edges in graph.items():
        iti[v] = v_ctr
        v_ctr += 1
        for edge in edges:
            iti[edge.label] = e_ctr
            e_ctr += 1
    return iti 

# Returns a string of the graph in DOT format. To view a file in DOT format,
# use `dot -Tps file.dot -o output.ps`.
def graph_to_dot(infile):
    graph, metadata = process_json(infile)
    iti = identifier_to_int(graph)
    s = ["digraph prov {"]
    for v, edges in graph.items():
        s.extend(['\t"%s" -> "%s" [label="%s"];' % (
            str(metadata[v].data['cf:type']) + ", " 
                + str(metadata[v].data['cf:id']) + ', ' 
                + str(iti[v]), 
            str(metadata[edge.dest].data['cf:type']) + ", " 
                + str(metadata[edge.dest].data['cf:id']) + ', '
                + str(iti[edge.dest]),
            metadata[edge.label].data['cf:type'])
            for edge in edges])
    s.append("}")
    return "\n".join(s)

# Returns a string of the graph in gspan format. 
# Run gSpan -f provgspan -s 0.1 -o -i. Output will be located in provspan.fp
# All gSpan input needs to be ints: vertex ids, edge ids, and typ ids are 
# all mapped to ints and printed out.
def graph_to_gspan(infile):
    graph, metadata = process_json(infile)
    iti = identifier_to_int(graph)
    s = ["t # 0"]
    vs = []
    es = []
    for v, edges in graph.items():
        vs.extend(['v %s %s' % (iti[v], RECOGNIZED_TYPS.index(metadata[v].typ))])
        es.extend(['e %s %s %s' % (iti[v], iti[edge.dest], 0) for edge in edges])
    return "\n".join(s + vs + es)

# Returns a string representing the compressed node metadata for the graph.
def compress_node_metadata(infile):
    '''
    Example output:
        The first substring sent is the reference string.
        Node keys are as defined below (in that order)
        Data corresponding to keys are separated by a comma
        Extra keys/data pairs are identified with a $
        A new relation is identified with a #
        Undefined identifiers (with no metadata) are identified with a ?
        If metadata corresponds to the metadata of a previous identifier with the same ID, 
            it is encoded as int(identifier)@index_of_table(first_identifier)

    None,457525,4,1516773907,734854839,9,2016:11:03T22:07:06.978,[]#
    2,0,0,0,0,0,0,0,cf:gid$1000,cf:uid$1000,prov:label$[task] 9#
    4,?#
    42,?#
    11,5,0,0,0,1,2016:11:03T22:07:09.119,0,cf:gid$1000,cf:uid$1000,prov:label$[task] 10#
    35,?#
    19@4,0,0,0,0,2,0,0,0,0,prov:label$[task] 11#
    '''

    graph, metadata = process_json(infile)
    # XXX TODO need to send this over as well
    iti = identifier_to_int(graph)
    id_dict = {}
    node_keys = {
            'cf:id':1,
            'cf:type':2,
            'cf:boot_id':3,
            'cf:machine_id':4,
            'cf:version':5,
            'cf:date':6,
            'cf:taint':7,
    }

    compressed_nodes = []
    default_node_data = [None for _ in range(len(node_keys)+1)]
    have_set_default = False
    for identifier, data in metadata.items():
        if data.typ == "relation":
            continue
        if data.data['cf:type'] == None:
            node_data = [str(iti[identifier]), '?']
        else:
            node_data = [None for _ in range(len(node_keys)+1)]

            # encode the data with reference to the default string
            node_data[0] = iti[identifier]
            for key, datum in data.data.items():
                val = datum 
                # this key is not in the default metadata for nodes
                if key not in node_keys:
                    node_data.append(str(key) + '$' + str(val))
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

            cf_id = data.data['cf:id']
            # store the index of the first time we see this ID and the corresponding metadata
            if cf_id not in id_dict:
                id_dict[cf_id] = (len(compressed_nodes), node_data)
            else:
                # this id has had metadata defined for it before. compress with reference to this metadata
                if cf_id in id_dict:
                    id_data = id_dict[cf_id][1]
                    node_data[0] = str(node_data[0]) +'@'+str(id_dict[cf_id][0])
                    for i, val in enumerate(node_data):
                        # delta encode with reference to the id node data
                        if val == id_data[i]:
                            node_data[i] = '0'

            if not have_set_default:
                have_set_default = True
        compressed_nodes.append(node_data)
    
    default_node_data = list(map(str, default_node_data))
    s = [','.join(default_node_data)]
    for ls in compressed_nodes:
        s.append(','.join(ls))
    s = '#'.join(s)
    return s

# Returns a string representing the compressed relation metadata for the graph.
# Currently, only integer references are compressed (delta-encoded against the default)
# Strings (including time) are written without compression
def compress_relation_metadata(infile):
    '''
    Example output:
        Relation keys are as defined below (in that order)
        Data corresponding to keys are separated by a comma
        Extra keys/data pairs are identified with a $
        A new relation is identified with a #.

        Default: 160,1516773907,734854839,None,2016:11:03T22:07:09.119,[],open,open,true,36,45#
        0,0,0,None,0,0,0,0,0,0,0#
        2,0,0,None,0,0,read,read,0,0,-44#
        -21,0,0,None,2016:11:03T22:07:06.978,0,version,version,0,-30,-16
    '''

    graph, metadata = process_json(infile)
    # XXX TODO need to send this over as well
    iti = identifier_to_int(graph)
    
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
    for identifier, data in metadata.items():
        if data.typ != "relation":
            continue
        # using lists to ensure that the keys and relation data are in the same order
        relation_data = [None for _ in range(len(relation_keys)+1)]
        relation_data[0] = iti[identifier]
        for key, datum in data.data.items():
            val = datum 
            # this key is not in the default metadata for relations
            if key not in relation_keys:
                relation_data.append(str(key) + '$' + str(val))
            else:
                # we want to not encode the entire identifier---just map to ints
                if key == 'cf:sender' or key == 'cf:receiver':
                    val = iti[datum]
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
        relation_data = list(map(str,relation_data))
        compressed_relations.append(relation_data)
    default_relation_data = list(map(str, default_relation_data))
    s = [','.join(default_relation_data)]
    for ls in compressed_relations:
        s.append(','.join(ls))
    s = '#'.join(s)
    return s

def main():
    if len(sys.argv) < 2:
        print("No input file: defaulting to /tmp/audit.log.")
        infile = "/tmp/audit.log"
    elif len(sys.argv) == 2:
        infile = sys.argv[1]
    else:
        print("Cannot supply more than one input file.")
        sys.exit(1)
    #dots_input = graph_to_dot(infile)
    #gspan_input = graph_to_gspan(infile)
    #print(compress_relation_metadata(infile))
    print(compress_node_metadata(infile))

if __name__ == "__main__":
    main()
