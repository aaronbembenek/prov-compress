#!/usr/bin/python3

from graph import Graph
from preprocess_v2 import clean_camflow_json, is_version_edge, PreprocessorV2
from util import nbits_for_int, WriterBitString

class GraphCompressorV2:

    def __init__(self, ppv2):
        assert isinstance(ppv2, PreprocessorV2)
        self.pp = ppv2
        self.is_initialized = False

    def compress(self):
        (graph, collapsed, uf) = self.pp.process()
        (afwd, aback, collapsed_nodes) = self._construct_asymmetric_graphs(
            graph, collapsed, uf)
        (delta_fwd, fwd_stats) = self._delta_encode_graph(afwd,
                                                          collapsed_nodes)
        (delta_back, back_stats) = self._delta_encode_graph(aback,
                                                            collapsed_nodes)

        header_byts = self._compress_header(fwd_stats, back_stats)
        (node_byts, index) = self._compress_nodes(delta_fwd, fwd_stats,
                                                  delta_back, back_stats,
                                                  collapsed_nodes, uf)

        nbits_index_entry = nbits_for_int(max(index))
        header_byts.extend(nbits_index_entry.to_bytes(1, byteorder="big"))
        header_byts.extend(len(index).to_bytes(4, byteorder="big"))
        wbs = WriterBitString()
        for idx in index:
            wbs.write_int(idx, nbits_index_entry)
        header_byts.extend(wbs.to_bytearray())
        
        self.header_byts = header_byts
        self.node_byts = node_byts

        self.is_initialized = True

    def _construct_asymmetric_graphs(self, graph, collapsed, uf):
        asym_fwd = Graph()
        asym_back = Graph()
        collapsed_nodes = set()
        for node in collapsed.get_vertices():
            assert uf.find(node) == node
            base = self.pp.id2num(node)
            asym_fwd.add_vertex(base)
            asym_back.add_vertex(base)
            n = uf.get_size(node)
            if n > 1:
                collapsed_nodes.add(base)
            for i in range(uf.get_size(node)):
                cur = base + i
                edges = graph.get_outgoing_edges(self.pp.num2id(cur))
                for edge in edges:
                    if is_version_edge(edge, self.pp.get_metadata()):
                        continue
                    dest = edge.dest
                    asym_fwd.add_edge(base, self.pp.id2num(dest))
                    dest_base = self.pp.id2num(uf.find(dest))
                    asym_back.add_vertex(dest_base)
                    asym_back.add_edge(dest_base, cur)
        return (asym_fwd, asym_back, collapsed_nodes)

    def _delta_encode_graph(self, graph, collapsed_nodes):
        delta_graph = Graph()
        stats = {}
        for x in [True, False]:
            stats[x] = {"nbits_degree":1, "nbits_delta":1}
        for node in graph.get_vertices():
            delta_graph.add_vertex(node)
            edges = graph.get_outgoing_edges(node)
            if not edges:
                continue
            c = node in collapsed_nodes
            stats[c]["nbits_degree"] = max(stats[c]["nbits_degree"],
                                         nbits_for_int(len(edges)))
            prev = node
            for edge in sorted(edges):
                delta_graph.add_edge(node, edge - prev)
                prev = edge
            abs_deltas = [abs(x) for x in delta_graph.get_outgoing_edges(node)]
            stats[c]["nbits_delta"] = max(stats[c]["nbits_delta"],
                                            nbits_for_int(max(abs_deltas)))
        return (delta_graph, stats)

    def _compress_header(self, fwd_stats, back_stats):
        byts = bytearray()
        for stats in fwd_stats, back_stats:
            for is_collapsed in [True, False]:
                byts.extend(stats[is_collapsed]["nbits_degree"].to_bytes(
                    1, byteorder="big"))
                byts.extend(stats[is_collapsed]["nbits_delta"].to_bytes(
                    1, byteorder="big"))
        return byts

    def _compress_nodes(self, delta_fwd, fwd_stats, delta_back, back_stats,
                  collapsed_nodes, uf):
        index = [0]
        sizes = {self.pp.id2num(k):uf.get_size(k) for k in uf.leaders()}
        nbits_size_entry = nbits_for_int(max(sizes.values()))
        wbs = WriterBitString()
        prev = 0 
        for node in sorted(delta_fwd.get_vertices()):
            c = node in collapsed_nodes 
            length = wbs.write_bit(1 if c else 0)
            if c:
                length += wbs.write_int(sizes[node], nbits_size_entry)
            length += self._compress_edges(node, delta_fwd, fwd_stats[c], wbs)
            length += self._compress_edges(node, delta_back, back_stats[c], wbs)
            index.append(length)
            prev = node
        index.pop()
        return (wbs.to_bytearray(), index)

    def _compress_edges(self, node, delta_graph, stats, wbs):
        length = 0
        nbits_degree = stats["nbits_degree"]
        edges = delta_graph.get_outgoing_edges(node)
        length += wbs.write_int(len(edges), nbits_degree)
        if edges:
            nbits_delta = stats["nbits_delta"]
            first_delta = edges[0]
            if first_delta < 0:
                length += wbs.write_int(abs(first_delta) * 2 + 1,
                                        nbits_delta + 1)
            else:
                length += wbs.write_int(first_delta * 2, nbits_delta + 1)
            for edge in edges[1:]:
                length += wbs.write_int(edge, nbits_delta)
        return length

def main():
    with open("example.json") as f:
        json_obj = " ".join([line.strip() for line in f])
        json_obj = clean_camflow_json(json_obj.split("SPLIT"))
    pp = PreprocessorV2(json_obj)
    gc = GraphCompressorV2(pp)
    gc.compress()

if __name__ == "__main__":
    main()
