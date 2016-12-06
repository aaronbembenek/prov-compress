#!/usr/bin/python3

'''
Given a JSON file with Camflow-style provenance data, constructs a provenance
graph and a dictionary of metadata.
Provides functions to output data in different formats for dot/gspan processing.
'''
import json
import os
import sys
import unionfind

PATH = "../compression"

DICT_BEGIN = '{'
DICT_END = '}'

RELATION_TYPS = ["wasGeneratedBy", "wasInformedBy", "wasDerivedFrom", "used", "relation"]
NODE_TYPS = ["prefix", "activity", "entity", "unknown"]

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
            i = line.find(DICT_BEGIN)
            if i == -1:
                continue
            line = line[i:]
            for typ, entries in json.loads(line).items():
                if typ == "prefix":
                    continue
                for identifier, data in entries.items():
                    # confirm that each entry in the JSON has a unique ID and metadata.
                    if identifier in metadata:
                        assert(typ == metadata[identifier].typ)
                        '''
                        for d in data.keys():
                            if (metadata[identifier].data[d] != data[d]):
                                pass # XXX 
                                #print("Different Metadata!")
                                #print(identifier, d, metadata[identifier].data[d], data[d])
                        '''
                    metadata[identifier] = Metadata(typ, data)
                    missing_nodes.discard(identifier)

                    # set up edges in graph
                    if typ in RELATION_TYPS:
                        if typ == 'used':
                            head = data["prov:entity"]
                            tail = data["prov:activity"]
                        elif typ == 'wasGeneratedBy':
                            head = data["prov:activity"]
                            tail = data["prov:entity"]
                        elif typ == 'wasDerivedFrom':
                            head = data["prov:usedEntity"]
                            tail = data["prov:generatedEntity"]
                        elif typ == 'wasInformedBy':
                            head = data["prov:informant"]
                            tail = data["prov:informed"]
                        elif typ == 'relation':
                            head = data["prov:sender"]
                            tail = data["prov:receiver"]
                        
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
        # confirm that each entry in the JSON has a unique ID and metadata.
        if identifier in metadata:
            assert(typ == metadata[identifier].typ)
            for d in data.keys():
                assert(metadata[identifier].data[d] == data[d])
        metadata[identifier] = Metadata("unknown", {'cf:id':-i, 'cf:type':None})
        graph.setdefault(identifier, [])
    return graph, metadata

# Returns a string of the graph in DOT format. To view a file in DOT format,
# use `dot -Tps file.dot -o output.ps`.
def graph_to_dot(infile, iti):
    graph, metadata = json_to_graph_data(infile)
    s = ["digraph prov {"]
    for v, edges in graph.items():
        s.extend(['\t"%s" -> "%s" [label="%s"];' % (
            str(metadata[v].typ) +", " +  
            str(metadata[v].data['cf:type']) + ", " 
                + str(metadata[v].data['cf:id']) + ', ' 
                + str(iti[v]), 
            str(metadata[edge.dest].typ) + ", " + str(metadata[edge.dest].data['cf:type']) + ", " 
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
        vs.extend(['v %s %s' % (iti[v], NODE_TYPS.index(metadata[v].typ))])
        es.extend(['e %s %s %s' % (iti[v], iti[edge.dest], 0) for edge in edges])
    return "\n".join(s + vs + es)

def graph_to_dot2(infile):
    graph, metadata = json_to_graph_data(infile)
    def node2str(node):
        data = metadata[node].data
        dtype = str(data["cf:type"])
        if dtype == "file_name":
            return dtype + ": " + data["cf:pathname"]
        return dtype + ", " + str(data["cf:id"]) + " " + node
    s = ["digraph prov {"]
    for v, edges in graph.items():
        s.extend(['\t"%s" -> "%s" [label="%s"];' % (
            node2str(v), 
            node2str(edge.dest),
            metadata[edge.label].typ + ", " +
            metadata[edge.label].data['cf:type'])
            for edge in edges])
    s.append("}")
    return "\n".join(s)

def graph_to_dot3(infile):
    graph, metadata = json_to_graph_data(infile)

    uf = unionfind.UnionFind(graph.keys()) 
    for v, edges in graph.items():
        for edge in edges:
            if metadata[edge.label].data["cf:type"] == "version":
                uf.union(edge.dest, v)

    def node2str(node):
        data = metadata[node].data
        dtype = str(data["cf:type"])
        if dtype == "file_name":
            return data["cf:pathname"]
        return dtype + ", " + uf.find(node)
        #return uf.find(node)
    already = set()
    s = ["digraph prov {"]
    for v, edges in graph.items():
        for edge in edges:
            if metadata[edge.label].data["cf:type"] == "version":
                continue
            pair = (uf.find(v), uf.find(edge.dest))
            if pair in already:
                continue
            already.add(pair)
            s.append('\t"%s" -> "%s";' % (
                node2str(v), 
                node2str(edge.dest)))#,
                #metadata[edge.label].typ + ", " +
                #metadata[edge.label].data['cf:type']))
    s.append("}")
    return "\n".join(s)


def main():
    print(graph_to_dot2(sys.argv[1]))

if __name__ == "__main__":
    main()
