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

# Returns a string representing the compressed metadata for the graph.
# Format is  
def compress_node_metadata(infile):
    graph, metadata = process_json(infile)
    iti = identifier_to_int(graph)
    index_of_id = {}

    v_ctr = 0
    node_keys_list = []
    node_keys_set = {}
    default_node_data = []
    # id, type, boot_id, machine_id, version, date, taint, and jiffies
    # are common fields of all nodes

# Returns a string representing the compressed relation metadata for the graph.
#   Relation keys are as defined below (in that order)
#   Data corresponding to keys are separated by a comma
#   Extra keys/data pairs are identified with a $
#   A new relation is identified with a #.
'''
e.g.
    Default: 160,1516773907,734854839,None,2016:11:03T22:07:09.119,[],open,open,true,36,45#
    0,0,0,None,0,0,0,0,0,0,0#
    2,0,0,None,0,0,read,read,0,0,-44#
    -21,0,0,None,2016:11:03T22:07:06.978,0,version,version,0,-30,-16
'''
# Currently, only integer references are compressed (delta-encoded against the default)
# Strings (including time) are written without compression
def compress_relation_metadata(infile):
    graph, metadata = process_json(infile)
    iti = identifier_to_int(graph)
    # id, boot_id, machine_id, version, date, taint, jiffies, 
    # type, label, allowed, sender, receiver are
    # common fields of all relations
    # offset is optional
    relation_keys = {
            'cf:id':0,
            'cf:boot_id':1,
            'cf:machine_id':2,
            'cf:date':3,
            'cf:taint':4,
            'cf:type':5,
            'prov:label':6,
            'cf:allowed':7,
            'cf:sender':8,
            'cf:receiver':9
    }
    compressed_relations = []
    default_relation_data = [None for _ in range(len(relation_keys))]
    for identifier, data in metadata.items():
        # used to identify when the default reference doesn't have
        # this metadata
        extra = []
        
        if data.typ == "relation":
            # using lists to ensure that the keys and relation data are in the same order
            relation_data = [None for _ in range(len(relation_keys))]

            for key, datum in data.data.items():
                val = datum 
                # this key is not in the default metadata for relations
                if key not in relation_keys:
                    relation_data.append(key + '$' + val)
                else:
                    # we want to not encode the entire identifier---just map to ints
                    if key == 'cf:sender' or key == 'cf:receiver':
                        val = iti[datum]
                    # set the default relation if there have been no relations
                    # compressed so far
                    if compressed_relations == []:
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
    print(compress_relation_metadata(infile))

if __name__ == "__main__":
    main()
