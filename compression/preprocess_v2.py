#!/usr/bin/python3

from collections import deque
from graph import LabeledBackEdgeGraph
import json
import sys
from unionfind import CustomUnionFind
from util import nbits_for_int, warn

RELATION_TYPS = {
    "used": ("prov:entity", "prov:activity"),
    "wasGeneratedBy": ("prov:activity", "prov:entity"),
    "wasDerivedFrom": ("prov:usedEntity", "prov:generatedEntity"),
    "wasInformedBy": ("prov:informant", "prov:informed"),
    "relation": ("prov:sender", "prov:receiver")
}


class MetadataEntry:

    def __init__(self, typ, data):
        self.typ = typ
        self.data = data

    def __repr__(self):
        return '(typ = "%s", data = "%s")' % (self.typ, self.data)


def clean_camflow_json(messy_json):
    clean = []
    for line in messy_json:
        i = line.find("{")
        if i == -1:
            continue
        clean.append(json.loads(line[i:]))
    return clean


def is_version_edge(edge, metadata):
    return metadata[edge.label].data["cf:type"] == "version"

class PreprocessorV2:

    def __init__(self, json_obj):
        self.json_obj = json_obj
        self.is_initialized = False

    def process(self):
        if self.is_initialized:
            return (self.graph, self.collapsed, self.uf)

        graph = self._process_json()
        (collapsed, uf) = self._collapse_versions(graph)
        self._number_identifiers(graph, collapsed, uf)

        self.collapsed = collapsed
        self.graph = graph
        self.uf = uf
        self.is_initialized = True
        return self.process() 

    def _process_json(self):
        self.metadata = {}
        graph = LabeledBackEdgeGraph()
        missing = set()
        for line in self.json_obj:
            self._process_line(line, graph, missing)
        # Handle nodes that are referenced as parts of arcs but do not have
        # entries in the metadata.
        for i, id_ in enumerate(missing):
            assert id_ not in self.metadata
            self.metadata[id_] = MetadataEntry("unknown", {})
            graph.add_vertex(id_)
        return graph

    def _check_id(self, id_, data, typ):
        if id_ in self.metadata:
            assert typ == self.metadata[id_].typ
            for d in data.keys():
                if self.metadata[id_].data[d] != data[d]:
                    warn("Different Metadata!")
                    warn(id_, d, self.metadata[id_].data[d], data[d])

    def _process_line(self, line, graph, missing):
        for (typ, entries) in line.items():
            if typ == "prefix":
                continue
            for (id_, data) in entries.items():
                # Confirm that each entry has a unique ID and metadata.
                self._check_id(id_, data, typ)
                self.metadata[id_] = MetadataEntry(typ, data)
                missing.discard(id_)

                # Set up edges in graph.
                if typ in RELATION_TYPS:
                    head = data[RELATION_TYPS[typ][0]]
                    tail = data[RELATION_TYPS[typ][1]]

                    # There are relations defined where the sender ID
                    # is never actually defined as an entity/activity;
                    # add empty metadata for these.
                    for v in [head, tail]:
                        if v not in self.metadata:
                            missing.add(v)
                    graph.add_vertex(head)
                    graph.add_vertex(tail)
                    if head not in [e.dest for e
                                    in graph.get_outgoing_edges(tail)]:
                        graph.add_edge(tail, head, id_)
                    else:
                        warn("Multiple edges from", tail, "to", head)
                else:
                    # This is just so that we have a node in the graph
                    # for every entity/activity.
                    graph.add_vertex(id_)

    def _collapse_versions(self, graph):
        vs = graph.get_vertices()
        uf = CustomUnionFind(vs)
        for v in vs:
            for edge in graph.get_outgoing_edges(v):
                if is_version_edge(edge, self.metadata):
                    uf.union(edge.dest, v)
        edge_map = {}
        collapsed = LabeledBackEdgeGraph()
        for v in vs:
            v = uf.find(v)
            edge_map.setdefault(v, set())
            collapsed.add_vertex(v)
            for edge in graph.get_outgoing_edges(v):
                u = uf.find(edge.dest)
                if u not in edge_map[v]:
                    edge_map[v].add(u)
                    collapsed.add_vertex(u)
                    collapsed.add_edge(v, u, None)
        return (collapsed, uf)

    def _number_identifiers(self, graph, collapsed, uf):
        sizes = {x:uf.get_size(x) for x in uf.leaders()}
        ranker = TransposeBfsRanker(graph, collapsed, self.metadata, sizes)
        id_map = ranker.rank()

        node_count = len(id_map)
        node_bits = nbits_for_int(node_count)
        for _id, entry in self.metadata.items():
            if entry.typ in RELATION_TYPS:
                head = entry.data[RELATION_TYPS[entry.typ][0]]
                tail = entry.data[RELATION_TYPS[entry.typ][1]]
                id_map[_id] = ((id_map[head] << node_bits)
                               + id_map[tail] + node_count)
        sorted_ids = sorted(id_map.keys(), key=lambda v: id_map[v])
        with open("identifiers.txt", 'wb') as f:
            f.write(node_count.to_bytes(4, byteorder='big')) 
        with open("identifiers.txt", 'a') as f:
            for _id in sorted_ids:
                f.write(_id + ",")
        self.id_map = id_map
        self.id_map_rev = {v:k for (k, v) in id_map.items()} 

    def id2num(self, _id):
        assert self.is_initialized
        return self.id_map[_id]

    def num2id(self, number):
        assert self.is_initialized
        return self.id_map_rev[number]

    def get_graph(self):
        assert self.is_initialized
        return self.graph

    def get_metadata(self):
        assert self.is_initialized
        return self.metadata

    def get_id2num_map(self):
        assert self.is_initialized
        return self.id_map


class TransposeBfsRanker:

    def __init__(self, graph, collapsed, metadata, sizes):
        self.graph = graph
        self.collapsed = collapsed
        self.metadata = metadata
        self.sizes = sizes

    def rank(self):
        order = self._get_visiting_order()
        rank = 0
        self.rankings = {}
        q = deque()
        for v in order:
            if v not in self.rankings:
                q.append(v)
            while q:
                v = q.popleft()
                rank = self._rank_collapsed(v, rank)
                dests = [e.dest for e in self.collapsed.get_incoming_edges(v)]
                for d in dests:
                    if d not in self.rankings:
                        q.append(d)
        return self.rankings

    def _rank_collapsed(self, v, rank):
        cur = [v]
        while cur:
            cur = cur[0]
            assert cur not in self.rankings
            self.rankings[cur] = rank
            rank += 1
            cur = [e.dest for e in self.graph.get_incoming_edges(cur)
                   if is_version_edge(e, self.metadata)]
            assert len(cur) < 2
            #if len(cur) > 1:
            #    warn("multiple versions", cur)
        return rank 

    # XXX placeholder... but maybe will be good, depending on how CamFlow
    # assigns identifiers?
    def _get_visiting_order(self):
        return sorted(self.collapsed.get_vertices())

    # XXX this terminates in the presence of cycles but does not assign scores
    # correctly. To fix this, need to collapse connected components and run
    # algorithm on that graph.
#    def _get_visiting_order(self):
#        scores = {} 
#        for v in self.graph.get_vertices():
#            if v in scores:
#                continue
#            local_visited = {v}
#            stack = [(v, 0, 1)]
#            forwards = True
#            acc = 0 
#            while stack:
#                assert not forwards or not acc
#                (v, i, local_acc) = stack[-1]
#                assert forwards or v not in scores
#                if not forwards:
#                    local_acc += acc
#                    stack[-1] = (v, i, local_acc) 
#                    acc = 0
#                elif v in scores:
#                    forwards = False
#                    acc = scores[v]
#                    stack.pop()
#                    continue
#
#                edges = self.graph.get_outgoing_edges(v)
#                if i < len(edges):
#                    forwards = True
#                    stack[-1] = (v, i + 1, local_acc)
#                    dest = edges[i].dest
#                    if dest not in local_visited:
#                        stack.append((dest, 0, 1))
#                        local_visited.add(dest)
#                else:
#                    forwards = False
#                    stack.pop()
#                    scores[v] = local_acc
#                    acc = local_acc
#        print(scores)
#        return sorted(scores.keys(), key=lambda v: -scores[v])

def main():
    with open("example.json") as f:
        json_obj = " ".join([line.strip() for line in f])
        json_obj = clean_camflow_json(json_obj.split("SPLIT"))
    pp = PreprocessorV2(json_obj)
    pp.process()

if __name__ == "__main__":
    main()
