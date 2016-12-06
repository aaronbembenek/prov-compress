#!/usr/bin/python3

from graph import Graph
from preprocess_v2 import clean_camflow_json, is_version_edge, PreprocessorV2
from util import nbits_for_int, ReaderBitString, WriterBitString

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
        sizes = {self.pp.id2num(k):uf.get_size(k) for k in uf.leaders()}
        nbits_size_entry = nbits_for_int(max(sizes.values()))

        header_byts = self._compress_header(fwd_stats, back_stats,
                                            nbits_size_entry)
        (node_byts, index) = self._compress_nodes(delta_fwd, fwd_stats,
                                                  delta_back, back_stats,
                                                  collapsed_nodes)

        nbits_index_entry = nbits_for_int(max(index))
        header_byts.extend(nbits_index_entry.to_bytes(1, byteorder="big"))
        header_byts.extend(len(index).to_bytes(4, byteorder="big"))
        wbs = WriterBitString()
        for (idx, sz) in zip(index, [sizes[x] for x in sorted(sizes.keys())]):
            wbs.write_int(idx, nbits_index_entry)
            wbs.write_int(sz, nbits_size_entry)
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

    def _compress_header(self, fwd_stats, back_stats, nbits_size_entry):
        byts = bytearray()
        for stats in fwd_stats, back_stats:
            for is_collapsed in [True, False]:
                byts.extend(stats[is_collapsed]["nbits_degree"].to_bytes(
                    1, byteorder="big"))
                byts.extend(stats[is_collapsed]["nbits_delta"].to_bytes(
                    1, byteorder="big"))
        byts.extend(nbits_size_entry.to_bytes(1, byteorder="big"))
        return byts

    def _compress_nodes(self, delta_fwd, fwd_stats, delta_back, back_stats,
                        collapsed_nodes):
        index = [0]
        szs = []
        wbs = WriterBitString()
        prev = 0
        for node in sorted(delta_fwd.get_vertices()):
            c = node in collapsed_nodes
            length = self._compress_edges(node, delta_fwd, fwd_stats[c], wbs)
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

    def write_to_file(self, base_filename):
        assert self.is_initialized
        with open(base_filename + ".cpg2", "wb") as f:
            f.write(self.header_byts)
            f.write(self.node_byts)

    def decompress(self):
        assert self.is_initialized
        rbs = ReaderBitString(self.header_byts)
        fwd_info = {True:{}, False:{}}
        back_info = {True:{}, False:{}}
        for info in [fwd_info, back_info]:
            for is_collapsed in [True, False]:
                d = info[is_collapsed]
                d["nbits_degree"] = rbs.read_int(8)
                d["nbits_delta"] = rbs.read_int(8)
        nbits_size_entry = rbs.read_int(8)
        nbits_index_entry = rbs.read_int(8)
        index_length = rbs.read_int(32)

        cur = 0
        index = []
        sizes = []
        for _ in range(index_length):
            cur += rbs.read_int(nbits_index_entry)
            index.append(cur)
            sizes.append(rbs.read_int(nbits_size_entry))

        id2idx = {}
        if index:
            id2idx[0] = 0
        id_ = 0
        for (idx, sz) in zip(index[1:], sizes[:-1]):
            id_ += sz
            id2idx[id_] = idx
        node_count = id_  + sizes[-1]

        return CompressedGraph(fwd_info, back_info, id2idx, node_count,
                               self.node_byts)


class CompressedGraph:
    def __init__(self, fwd_info, back_info, id2bit, node_count, node_byts):
        self.fwd_info = fwd_info
        self.back_info = back_info
        self.id2bit = id2bit
        self.rbs = ReaderBitString(node_byts)
        self.index = sorted(id2bit.keys())
        self.index.append(node_count)

    def __len__(self):
        return self.index[-1]

    def _get_leader_node(self, node):
        assert node >= 0 and node < self.index[-1]
        for (i, _id) in enumerate(self.index[::-1]):
            if _id <= node:
                return (len(self.index) - i - 1, _id)

    def _get_node_size(self, idx):
        return self.index[idx + 1] - self.index[idx]

    def _read_edges_raw(self, node, pos, info):
        degree = self.rbs.read_int_at_pos(pos, info["nbits_degree"])
        if not degree:
            return []
        first_delta = self.rbs.read_int(info["nbits_delta"] + 1)
        if first_delta % 2 == 0:
            first_delta //= 2
        else:
            first_delta = -(first_delta - 1) // 2
        edges = [node + first_delta]
        for _ in range(degree - 1):
            delta = self.rbs.read_int(info["nbits_delta"])
            edges.append(edges[-1] + delta)
        return edges

    def _get_outgoing_edges_raw(self, idx):
        node = self.index[idx]
        c = self._get_node_size(idx) > 1
        byte_pos = self.id2bit[node]
        return self._read_edges_raw(node, byte_pos, self.fwd_info[c]) 

    def _get_incoming_edges_raw(self, idx):
        node = self.index[idx]
        c = self._get_node_size(idx) > 1
        byte_pos = self.id2bit[node]
        degree = self.rbs.read_int_at_pos(byte_pos,
                                          self.fwd_info[c]["nbits_degree"])
        byte_pos += self.fwd_info[c]["nbits_degree"]
        if degree:
            byte_pos += self.fwd_info[c]["nbits_delta"] + 1
            byte_pos += (degree - 1) * self.fwd_info[c]["nbits_delta"]
        return self._read_edges_raw(node, byte_pos, self.back_info[c])

    def _get_edges(self, node, is_fwd, get_my_edges, get_other_edges):
        idx, leader = self._get_leader_node(node)
        byt_pos = self.id2bit[leader]
        sz = self._get_node_size(idx)
        c = sz > 1
        raw_edges = get_my_edges(idx)
        if not c:
            return raw_edges

        my_output = []
        my_low = leader
        my_hi = my_low + sz
        my_idx = 0

        if is_fwd and node - 1 >= my_low:
            my_output.append(node - 1)
        elif (not is_fwd) and node + 1 < my_hi:
            my_output.append(node + 1)

        while my_idx < len(raw_edges):
            (other_idx, other_leader) = self._get_leader_node(
                raw_edges[my_idx])
            other_low = other_leader
            other_hi = other_low + self._get_node_size(other_idx)
            other_edges = get_other_edges(other_idx)

            other_idx = 0
            while other_idx < len(other_edges):
                edge = other_edges[other_idx]
                if edge < my_low:
                    other_idx += 1
                    continue
                if edge >= my_hi:
                    break
                if edge == node:
                    my_output.append(raw_edges[my_idx])
                my_idx += 1
                other_idx += 1
        return sorted(my_output)

    def get_outgoing_edges(self, node):
        return self._get_edges(node, True, self._get_outgoing_edges_raw,
                               self._get_incoming_edges_raw)

    def get_incoming_edges(self, node):
        return self._get_edges(node, False, self._get_incoming_edges_raw,
                               self._get_outgoing_edges_raw)


def main():
    with open("example.json") as f:
        json_obj = " ".join([line.strip() for line in f])
        json_obj = clean_camflow_json(json_obj.split("SPLIT"))
    pp = PreprocessorV2(json_obj)
    gc = GraphCompressorV2(pp)
    gc.compress()
    g = gc.decompress()
    for i in range(len(g)):
        print(i, g.get_outgoing_edges(i), g.get_incoming_edges(i))
    gc.write_to_file("trial")

if __name__ == "__main__":
    main()
