#!/usr/bin/python3

import abc
import process_json as pj
import preprocess
import sys
import util

class Compressor(metaclass=abc.ABCMeta):

    def __init__(self, preprocessor):
        self.pp = preprocessor
        self.compressed = None

    @abc.abstractmethod
    def compress(self):
        pass

    def write_to_file(self, base_filename, ext="cpg"):
        if not self.compressed:
            self.compress()
        with open(base_filename + "." + ext, "wb") as f:
            f.write(self.compressed)

class BasicCompressor(Compressor):
    
    def compress(self):
        # Write header
        self.compressed = bytearray()
        info = {}
        for t in (False, True):
            degrees = [abs(x) for x in self.pp.get_degrees(transpose=t)]
            deltas = [abs(x) for x in self.pp.get_deltas(transpose=t)]
            nbits_degree = util.nbits_for_int(max(degrees))
            nbits_delta = util.nbits_for_int(max(deltas))
            graph = self.pp.get_graph(transpose=t)
            info[t] = (nbits_degree, nbits_delta, graph)
            self.compressed.extend(nbits_degree.to_bytes(1, byteorder="big"))
            self.compressed.extend(nbits_delta.to_bytes(1, byteorder="big"))
            #print(max(self.pp.get_degrees(transpose=t)))
            #print(max(self.pp.get_deltas(transpose=t)))
        #print(info)

        compressed_nodes, index = self.compress_nodes(info[False], info[True])

        nbits_index_entry = util.nbits_for_int(max(index))
        self.compressed.extend(nbits_index_entry.to_bytes(1, byteorder="big"))
        self.compressed.extend(len(index).to_bytes(4, byteorder="big"))
        bs = util.WriterBitString()
        for i in index:
            bs.write_int(i, width=nbits_index_entry)
        self.compressed.extend(bs.to_bytearray())
        self.compressed.extend(compressed_nodes)

    def compress_nodes(self, outinfo, ininfo):
        order = self.pp.rank()
        bs = util.WriterBitString()
        index = []
        for node in sorted(order.keys(), key=lambda v: order[v]):
            #print("node", node)
            length = 0
            rank = order[node]
            for i, (nbits_degree, nbits_delta, g) in enumerate((outinfo, ininfo)):
                #print(i, nbits_degree, nbits_delta)
                length += nbits_degree
                edges = [order[e.dest] for e in g[node]]
                degree = len(edges)
                #print("degree", degree)
                bs.write_int(degree, nbits_degree) 
                if degree == 0:
                    continue 
                edges.sort()
                first_delta = edges[0] - rank
                #print("first_delta", first_delta)
                nbits_first_delta = nbits_delta + 1
                if first_delta < 0:
                    bs.write_int(abs(first_delta) * 2 + 1,
                            width=nbits_first_delta)
                elif first_delta > 0:
                    bs.write_int(first_delta * 2, width=nbits_first_delta)
                else:
                    assert False
                length += nbits_first_delta 
                for i in range(degree - 1):
                    #print(edges[i+1] - edges[i])
                    bs.write_int(edges[i + 1] - edges[i], width=nbits_delta)
                    length += nbits_delta
            index.append(length)
        index.pop()
        return bs.to_bytearray(), index

    def decompress(self):
        if not self.compressed:
            self.compress()
        bs = util.ReaderBitString(self.compressed)
        info = {} 
        for x in ("out", "in"):
            nbits_degree = bs.read_int(8)
            nbits_delta = bs.read_int(8)
            info[x] = (nbits_degree, nbits_delta, {})
        """
        nbits_out_degree = bs.read_int(8)
        nbits_out_delta = bs.read_int(8)
        nbits_out_degree = bs.read_int(8)
        nbits_out_delta = bs.read_int(8)
        nbits_first_out_delta = nbits_out_delta + 1
        """
        nbits_index_entry = bs.read_int(8)
        index_length = bs.read_int(32)

        index = [0]
        for i in range(index_length):
            index.append(bs.read_int(nbits_index_entry) + index[-1])
        pad = (index_length * nbits_index_entry) % 8
        sizeof_index = index_length * nbits_index_entry
        pad = (((sizeof_index + 7) >> 3) << 3) - sizeof_index
        if pad:
            bs.read_int(pad)

        pos = 0
        count = 0
        id_to_identifier = {v:k for k, v in self.pp.rank().items()}
        while count < index_length + 1:
            assert index[count] == pos
            for x in ("out", "in"):
                (nbits_degree, nbits_delta, g) = info[x]
                degree = bs.read_int(nbits_degree)
                pos += nbits_degree
                edges = []
                if degree != 0:
                    nbits_first_delta = nbits_delta + 1
                    e0 = bs.read_int(nbits_first_delta)
                    pos += nbits_first_delta
                    if e0 % 2 == 0:
                        e0 //= 2
                    else:
                        e0 = -(e0 - 1) // 2
                    edges.append(count + e0)
                    for _ in range(degree - 1):
                        edges.append(edges[-1] + bs.read_int(nbits_delta))
                        pos += nbits_delta
                g[id_to_identifier[count]] = [id_to_identifier[e] for e in edges]
            count += 1
        return info["out"][2], info["in"][2], index

def main():
    if len(sys.argv) < 2:
        # Tiny DAG
        #           2
        #          / \
        #         4   3   7
        #        / \ /    |
        #       5   0     6
        #           |
        #           1
        # (all edges point from bottom up)
        #g = {1:[0], 0:[4,3], 2:[], 3:[2], 4:[2], 5:[4], 6:[7], 7:[]}
        #g = {v:[pj.Edge(e, None) for e in edges] for v, edges in g.items()}
        #metadata = {}

        # Tiny forest 
        #         2
        #         |
        #         4   3   7
        #        / \ /    |
        #       5   0     6
        #           |
        #           1
        # (all edges point from bottom up)
        g = {1:[0], 0:[4,3], 2:[], 3:[], 4:[2], 5:[4], 6:[7], 7:[]}
        g = {v:[pj.Edge(e, None) for e in edges] for v, edges in g.items()}
        metadata = {}
        filename = "trial"
    else:
        g, metadata = pj.json_to_graph_data(sys.argv[1])
        filename = sys.argv[1].split("/")[-1].rsplit(".", 1)[0]
    c = BasicCompressor(preprocess.BfsPreprocessor(g, metadata))
    c.compress()
    c.write_to_file(filename)
    #print(c.decompress())

if __name__ == "__main__":
    main()
