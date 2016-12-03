#!/usr/bin/python3

from graph import Graph
import json
import sys
from unionfind import CustomUnionFind
from util import warn

RELATION_TYPS = {
    "wasGeneratedBy": ("prov:entity", "prov:activity"),
    "wasInformedBy": ("prov:activity", "prov:entity"),
    "wasDerivedFrom": ("prov:usedEntity", "prov:generatedEntity"),
    "used": ("prov:informant", "prov:informed"),
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


class PreprocessorV2:

    def __init__(self, json_obj):
        self.json_obj = json_obj
        self.initialized = False

    def process(self):
        if self.initialized:
            return

        graph = self._process_json()
        (collapsed, uf) = self._collapse_versions(graph)
        order = self._get_visiting_order(collapsed)

        self.initialized = True

    def _process_json(self):
        self.metadata = {}
        graph = Graph()
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
                    warn(ident, d, self.metadata[ident].data[d], data[d])

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
                    graph.add_edge(tail, head, id_)
                else:
                    # This is just so that we have a node in the graph
                    # for every entity/activity.
                    graph.add_vertex(id_)

    def _collapse_versions(self, graph):
        vs = graph.get_vertices()
        uf = CustomUnionFind(vs)
        for v in vs:
            for edge in graph.get_outgoing_edges(v):
                if self.metadata[edge.label].data["cf:type"] == "version":
                    uf.union(edge.dest, v)
        edge_map = {}
        collapsed = Graph()
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

    # XXX this is going to be pretty memory intensive...
    def _get_visiting_order(self, graph):
        reachable_map = {} 
        for v in graph.get_vertices():
            if v in reachable_map:
                continue
            local_visited = set([v])
            stack = [(v, 0, set([v]))]
            forwards = True
            acc = set() 
            while stack:
                (v, i, local_acc) = stack[-1]
                assert forwards or v not in reachable_map
                if not forwards:
                    local_acc.update(acc)
                    acc = set()
                elif v in reachable_map:
                    assert len(acc) == 0
                    forwards = False
                    acc = reachable_map[v]
                    stack.pop()
                    continue

                edges = graph.get_outgoing_edges(v)
                if i < len(edges):
                    forwards = True
                    stack[-1] = (v, i + 1, local_acc)
                    dest = edges[i].dest
                    if dest not in local_visited:
                        stack.append((dest, 0, set([dest])))
                        local_visited.add(dest)
                else:
                    forwards = False
                    stack.pop()
                    reachable_map[v] = local_acc
                    acc = local_acc
        print(reachable_map)
        return sorted(reachable_map.keys(),
                      key=lambda v: -len(reachable_map[v]))


def main():
    with open("example.json") as f:
        json_obj = " ".join([line.strip() for line in f])
        json_obj = clean_camflow_json(json_obj.split("SPLIT"))
    pp = PreprocessorV2(json_obj)
    pp.process()

if __name__ == "__main__":
    main()
