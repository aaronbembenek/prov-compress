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

    def write_to_file(self, base_filename):
        if not self.compressed:
            self.compress()
        with open(base_filename + ".cpg", "wb") as f:
            f.write(self.compressed)

class BasicCompressor(Compressor):
    
    def compress(self):
        nbits_for_degree = util.nbits_for_int(max(self.pp.get_degrees()))
        nbits_for_delta = util.nbits_for_int(max(self.pp.get_deltas()))
        self.compressed = bytearray()
        self.compressed.extend(nbits_for_degree.to_bytes(1, byteorder="big"))
        self.compressed.extend(nbits_for_delta.to_bytes(1, byteorder="big"))
       
        compressed_nodes, index = self.compress_nodes(nbits_for_degree,
                nbits_for_delta)

        nbits_for_index = util.nbits_for_int(max(index))
        self.compressed.extend(nbits_for_index.to_bytes(1, byteorder="big"))
        self.compressed.extend(len(index).to_bytes(1, byteorder="big"))
        bs = util.WriterBitString()
        for i in index:
            bs.write_int(i, width=nbits_for_index)
        self.compressed.extend(bs.to_bytearray())
        self.compressed.extend(compressed_nodes)

    def compress_nodes(self, nbits_for_degree, nbits_for_delta):
        g = self.pp.get_graph()
        order = self.pp.rank()
        bs = util.WriterBitString()
        index = []
        for node in sorted(order.keys(), key=lambda v: order[v]):
            length = nbits_for_degree
            edges = [order[e.dest] for e in g[node]]
            outdegree = len(edges)
            bs.write_int(len(edges), nbits_for_degree) 
            if outdegree == 0:
                index.append(length)
                continue 
            rank = order[node]
            edges.sort()
            first_delta = edges[0] - rank
            nbits_for_first_delta = nbits_for_delta + 1
            if first_delta < 0:
                bs.write_int(abs(first_delta) * 2 + 1,
                        width=nbits_for_first_delta)
            elif first_delta > 0:
                bs.write_int(first_delta * 2, width=nbits_for_first_delta)
            else:
                assert False
            length += nbits_for_first_delta 
            for i in range(outdegree - 1):
                bs.write_int(edges[i + 1] - edges[i], width=nbits_for_delta)
                length += nbits_for_delta
            index.append(length)
        index.pop()
        return bs.to_bytearray(), index

    def decompress(self):
        if not self.compressed:
            self.compress()
        bs = util.ReaderBitString(self.compressed)
        nbits_for_degree = bs.read_int(8)
        nbits_for_delta = bs.read_int(8)
        nbits_for_first_delta = nbits_for_delta + 1
        nbits_for_index = bs.read_int(8)
        index_length = bs.read_int(8)

        index = [0]
        for i in range(index_length):
            index.append(bs.read_int(nbits_for_index) + index[-1])
        pad = (index_length * nbits_for_index) % 8
        sizeof_index = index_length * nbits_for_index
        pad = (((sizeof_index + 7) >> 3) << 3) - sizeof_index
        if pad:
            bs.read_int(pad)

        g = {}
        pos = 0
        count = 0
        id_to_identifier = {v:k for k, v in self.pp.rank().items()}
        while count < index_length:
            assert index[count] == pos
            degree = bs.read_int(nbits_for_degree)
            pos += nbits_for_degree
            edges = []
            if degree != 0:
                e0 = bs.read_int(nbits_for_first_delta)
                pos += nbits_for_first_delta
                if e0 % 2 == 0:
                    e0 //= 2
                else:
                    e0 = -(e0 - 1) // 2
                edges.append(count + e0)
                for _ in range(degree - 1):
                    edges.append(edges[-1] + bs.read_int(nbits_for_delta))
                    pos += nbits_for_delta
            g[id_to_identifier[count]] = [id_to_identifier[e] for e in edges]
            count += 1
        return g, index

def main():
    if len(sys.argv) < 2:
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
    else:
        g, metadata = pj.json_to_graph_data(sys.argv[1])
    c = BasicCompressor(preprocess.BfsPreprocessor(g, metadata))
    c.compress()
    c.write_to_file("trial")
    print(c.decompress())

if __name__ == "__main__":
    main()
