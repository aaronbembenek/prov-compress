#!/usr/bin/python3

# Given a JSON file with Camflow-style provenance data, constructs a provenance
# graph and a dictionary of metadata.

import json
import sys

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
                    # XXX see comment below for why this assertion is wrong
                    # XXX I want this assertion to be checked to confirm that
                    # we are correct in our assumption that each entry in the
                    # JSON has a unique ID.
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

# Returns a string of the graph in DOT format. To view a file in DOT format,
# use `dot -Tps file.dot -o output.ps`.
def graph_to_dot(graph, metadata):
    '''
    s = ["digraph prov {"]
    for v, edges in graph.items():
        s.extend(['\t"%s" -> "%s";' % (str(metadata[v].data['cf:id']) + ", " + str(metadata[v].data['cf:type']), str(metadata[edge.dest].data['cf:id']) + ", " + str(metadata[edge.dest].data['cf:type'])) for edge in edges])
    s.append("}")
    return "\n".join(s)
    '''
    ids_to_ints = {}
    v_ctr = 0
    e_ctr = 0
    for v, edges in graph.items():
        ids_to_ints[v] = v_ctr;
        v_ctr += 1
        for edge in edges:
            ids_to_ints[edge.label] = e_ctr;
            e_ctr += 1

    s = ["digraph prov {"]
    for v, edges in graph.items():
        s.extend(['\t"%s" -> "%s;"' % (str(metadata[v].data['cf:id']) + ', ' + str(ids_to_ints[v]), str(metadata[edge.dest].data['cf:id']) + ', '+ str(ids_to_ints[edge.dest])) for edge in edges])
    s.append("}")
    return "\n".join(s)

# Returns a string of the graph in gspan format. 
# Run gSpan -f provgspan -s 0.1 -o -i. Output will be located in provspan.fp
# All gSpan input needs to be ints: vertex ids, edge ids, and typ ids are 
# all mapped to ints and printed out.
def graph_to_gspan(graph, metadata):
    # initialize dictionaries mapping vertex/edge ids to ints
    ids_to_ints = {}
    v_ctr = 0
    e_ctr = 0
    for v, edges in graph.items():
        ids_to_ints[v] = v_ctr;
        v_ctr += 1
        for edge in edges:
            ids_to_ints[edge.label] = e_ctr;
            e_ctr += 1
    s = ["t # 0"]
    vs = []
    es = []
    for v, edges in graph.items():
        vs.extend(['v %s %s' % (ids_to_ints[v], RECOGNIZED_TYPS.index(metadata[v].typ))])
        es.extend(['e %s %s %s' % (ids_to_ints[v], ids_to_ints[edge.dest], 0) for edge in edges])
    return ids_to_ints, "\n".join(s + vs + es)

def main():
    if len(sys.argv) < 2:
        print("No input file: defaulting to /tmp/audit.log.")
        infile = "/tmp/audit.log"
    elif len(sys.argv) == 2:
        infile = sys.argv[1]
    else:
        print("Cannot supply more than one input file.")
        sys.exit(1)
    graph, metadata = process_json(infile)
    dots_input = graph_to_dot(graph, metadata)
    print dots_input
    #ids_to_ints, gspan_input = graph_to_gspan(graph, metadata)
    #print(gspan_input)

if __name__ == "__main__":
    main()
