#!/usr/bin/python3

import abc
import process_json as pj
import rank_nodes
import sys
import util

class Compressor(metaclass=abc.ABCMeta):
    def __init__(self, graph, preprocessor_factory, compressor_helper_factory):
        self.g = graph
        self.preprocessor_factory = preprocessor_factory
        self.compressor_helper_factory = compressor_helper_factory

    def compress(self):
        preprocessor = self.preprocessor_factory(self.g)
        ranking = preprocessor.rank()
        ch = self.compressor_helper_factory(self.g, preprocessor)
        bs = util.BitString()
        locations = [0] 
        for node in sorted(ranking.keys(), key=lambda v: ranking[v]):
            ch.compress_node(node, bs)
            locations.append(len(bs))
        locations.pop()
        byts = bs.to_bytearray()
        print(byts)
        print(len(byts))

    def write_to_file(self, file):
        if not self.compressed:
            self.compress()
        with open(file, "wb") as f:
            f.write("wb")

class CompressorHelper(metaclass=abc.ABCMeta):

    def __init__(self, graph, preprocessor):
        self.graph = graph
        self.preprocessor = preprocessor

    @abc.abstractmethod
    def compress_node(self, node, bs):
        pass

class BasicCompressorHelper(CompressorHelper):

    def __init__(self, graph, preprocessor):
        super().__init__(graph, preprocessor)
        delta = max(preprocessor.get_deltas())
        self.bits_for_delta = util.nbits_for_int(delta) 
        degree = max(preprocessor.get_degrees())
        self.bits_for_degree = util.nbits_for_int(degree) 

    def compress_node(self, node, bs):
        edges = self.graph[node]
        outdegree = len(edges)
        bs.write_int(outdegree, width=self.bits_for_degree)
        if outdegree == 0:
            return
        rankings = self.preprocessor.rank()
        rank = rankings[node]
        edges = [rankings[e.dest] for e in edges]
        edges.sort()
        first_delta = edges[0] - rank
        if first_delta < 0:
            bs.write_int(abs(first_delta) * 2 + 1, self.bits_for_delta + 2)
        elif first_delta > 0:
            bs.write_int(first_delta * 2, self.bits_for_delta + 2)
        else:
            assert False
        for i in range(outdegree - 1):
            bs.write_int(edges[i + 1] - edges[i], width=self.bits_for_delta)

class BasicCompressor(Compressor):

    def __init__(self, graph, preprocessor_factory):
        super().__init__(graph, preprocessor_factory,
                lambda g, pp: BasicCompressorHelper(g, pp))

def main():
    if len(sys.argv) < 2:
        test1()
    else:
        g, _ = pj.json_to_graph_data(sys.argv[1])
        c = BasicCompressor(g, lambda g: rank_nodes.BfsPreprocessor(g))
        c.compress()

if __name__ == "__main__":
    main()
