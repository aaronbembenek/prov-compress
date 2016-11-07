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
def json_to_graph_data(infile):
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
    graph, metadata = json_to_graph_data(infile)
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
    graph, metadata = json_to_graph_data(infile)
    iti = identifier_to_int(graph)
    s = ["t # 0"]
    vs = []
    es = []
    for v, edges in graph.items():
        vs.extend(['v %s %s' % (iti[v], RECOGNIZED_TYPS.index(metadata[v].typ))])
        es.extend(['e %s %s %s' % (iti[v], iti[edge.dest], 0) for edge in edges])
    return "\n".join(s + vs + es)

# Returns a string representing the compressed node metadata for the graph.
def compress_node_metadata(graph, metadata, iti):
    '''
    Example output:
        The first line is not part of the table: it is the default reference string
        ,-separated input are the data fields for the node keys (defined below)
        $: marks an extra keys/data pair
        #: marks a new node
        ?: marks undefined identifiers (with no metadata)
        a@b: marks when metadata is reference encoded off of the metadata of a previous identifier with the same ID
            a is the integer corresponding to the identifier of this node
            b is the index of the node in the list (i.e. the number of #s) of 
                the first instance of another identifier with the same ID as (a)

    None,457525,4,1516773907,734854839,9,2016:11:03T22:07:06.978,[]#
    2,0,0,0,0,0,0,0,cf:gid$1000,cf:uid$1000,prov:label$[task] 9#
    4,?#
    42,?#
    11,5,0,0,0,1,2016:11:03T22:07:09.119,0,cf:gid$1000,cf:uid$1000,prov:label$[task] 10#
    35,?#
    19@4,0,0,0,0,2,0,0,0,0,prov:label$[task] 11#
    '''
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

            # see if we can compress node_data further by referencing another node 
            # with the same camflow ID
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
    s = '{' + s + '}'
    return s

# Returns a string representing the compressed relation metadata for the graph.
# Currently, only integer references are compressed (delta-encoded against the default)
# Strings (including time) are written without compression
def compress_relation_metadata(graph, metadata, iti):
    '''
    Example output:
        The first line is not part of the table: it is the default reference string
        ,-separated input are the data fields for the relation keys (defined below)
        $: marks an extra keys/data pair
        #: marks a new relation

        160,1516773907,734854839,None,2016:11:03T22:07:09.119,[],open,open,true,36,45#
        0,0,0,None,0,0,0,0,0,0,0#
        2,0,0,None,0,0,read,read,0,0,-44#
        -21,0,0,None,2016:11:03T22:07:06.978,0,version,version,0,-30,-16
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

        # everything has to be strings...
        relation_data = list(map(str,relation_data))
        compressed_relations.append(relation_data)
    default_relation_data = list(map(str, default_relation_data))

    # join all strings together
    s = [','.join(default_relation_data)]
    for ls in compressed_relations:
        s.append(','.join(ls))
    s = '#'.join(s)
    s = '{' + s + '}'
    return s

# Write a dict mapping identifiers to ints, compressed relation metadata, and compressed
# node metadata to the provide outfile.
# Currently, only integer references are compressed (delta-encoded against the default)
# Strings (including time) are written without compression
def compress_metadata(infile, outfile="compressed.out"):
    graph, metadata = json_to_graph_data(infile)
    iti = identifier_to_int(graph)
    iti_str = str(iti).replace(' ', '').replace("'",'')
    crm = compress_relation_metadata(graph, metadata, iti) 
    cnm = compress_node_metadata(graph, metadata, iti) 
    with open(outfile,'w') as f:
        f.write(iti_str+crm+cnm)

def main():
    if len(sys.argv) == 1:
        print("No input file: defaulting to /tmp/audit.log.")
        infile = "/tmp/audit.log"
    elif len(sys.argv) == 2:
        infile = sys.argv[1]
    elif len(sys.argv) == 3:
        infile = sys.argv[1]
        outfile = sys.argv[2]
    else:
        print("Usage: ./process_json.py [infile] [outfile]")
        sys.exit(1)
    #dots_input = graph_to_dot(infile)
    #gspan_input = graph_to_gspan(infile)
    #print(compress_relation_metadata(infile))
    compress_metadata(infile)

if __name__ == "__main__":
    main()
