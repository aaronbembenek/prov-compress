#!/usr/bin/python3

import abc
import process_json as pj
import rank_nodes
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
        index = [0]
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
            if first_delta < 0:
                bs.write_int(abs(first_delta) * 2 + 1,
                        width=nbits_for_delta + 2)
            elif first_delta > 0:
                bs.write_int(first_delta * 2, width=nbits_for_delta + 2)
            else:
                assert False
            length += nbits_for_delta + 2  
            for i in range(outdegree - 1):
                bs.write_int(edges[i + 1] - edges[i], width=nbits_for_delta)
                length += nbits_for_delta
            index.append(length)
        index.pop()
        return bs.to_bytearray(), index 

def main():
    if len(sys.argv) < 2:
        #         4
        #         |
        #         2   3   7
        #        / \ /    |
        #       5   1     6
        #           |
        #           0
        # (all edges point from bottom up)
        g = {0:[1], 1:[2,3], 2:[4], 3:[], 4:[], 5:[2], 6:[7], 7:[]}
        g = {v:[pj.Edge(e, None) for e in edges] for v, edges in g.items()}
    else:
        g, _ = pj.json_to_graph_data(sys.argv[1])
    c = BasicCompressor(rank_nodes.BfsPreprocessor(g))
    c.compress()

if __name__ == "__main__":
    main()
