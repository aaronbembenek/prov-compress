#!/usr/bin/python3

import abc
import collections
import process_json as pj
import statistics
import sys
import util

# Abstract class for compression preprocesor.
class CompressionPreprocessor(metaclass=abc.ABCMeta):

    def __init__(self, graph, metadata):
        self.g = graph
        self.transpose = None
        self.metadata = metadata
        self.rankings = None
        self.out_deltas = None
        self.out_degrees = None
        self.in_deltas = None
        self.in_degrees = None
        self.ids = None

    # Return the nodes in decreasing "reachable order." The nodes are
    # ordered so that a node is earlier in the order the more nodes are
    # reachable from it. This is useful for identifying the order in which to
    # visit the sources during a graph search.
    def get_reachable_order(self):
        reachable_size = {}
        def get_reachable_size(v, visited, depth=0):
            # XXX hack to confine the depth of our search...
            # we can probably make it iterative 
            if depth > 200:
                return 0
            if v in reachable_size:
                return reachable_size[v]
            edges = [e.dest for e in self.g[v]]
            depth += 1
            x = len(edges)
            for edge in edges:
                x += get_reachable_size(edge, visited, depth)
            reachable_size[v] = x
            return x
        
        for node in self.g.keys():
            get_reachable_size(node, set())
        return sorted(self.g.keys(), key=lambda v: -reachable_size[v])

    def get_deltas(self, transpose=False):
        if not self.rankings:
            self.rank()
        if transpose:
            if self.in_deltas:
                return self.in_deltas
            if not self.transpose:
                self.transpose = util.transpose_graph(self.g)
            g = self.transpose
        else:
            if self.out_deltas:
                return self.out_deltas
            g = self.g

        visited = set()
        queue = collections.deque()
        deltas = []
        degrees = []
        for node in self.rankings.keys():
            if node not in visited:
                visited.add(node)
                queue.append(node)
            while queue:
                v = queue.popleft()
                degree = len(g[v])
                degrees.append(degree)
                if not degree:
                    continue
                for edge in g[v]:
                    e = edge.dest
                    #delta = abs(self.rankings[v] - self.rankings[e])
                    #deltas.append(delta)
                    if e not in visited:
                        visited.add(e)
                        queue.append(e)
                edges = sorted(self.rankings[e.dest] for e in g[v])
                deltas.append(edges[0] - self.rankings[v])
                for i in range(degree - 1):
                    deltas.append(edges[i + 1] - edges[i])
       
        deltas.sort()
        degrees.sort()
        if transpose:
            self.in_deltas = deltas
            self.in_degrees = degrees
        else:
            self.out_deltas = deltas
            self.out_degrees = degrees
        return deltas

    def cyclic(self):
        path = set()
        visited = set()

        def visit(vertex):
            if vertex in visited:
                return False
            visited.add(vertex)
            path.add(vertex)
            for edge in self.g.get(vertex, ()):
                if edge.dest in path or visit(edge.dest):
                    print(path, edge.dest)
                    return True
                path.remove(vertex)
            return False

        return(any(visit(v) for v in self.g))

    def get_degrees(self, transpose=False):
        if transpose:
            if not self.in_degrees:
                self.get_deltas(transpose=True)
            return self.in_degrees
        if not self.out_degrees:
            self.get_deltas()
        return self.out_degrees

    def to_dot(self, transpose=False):
        if not self.rankings:
            self.rank()
        if not self.ids:
            self.construct_identifier_ids()
        if transpose:
            if not self.transpose:
                self.transpose = util.transpose_graph(self.g)
            g = self.transpose
            s = ["digraph prov_transpose {"]
        else:
            g = self.g
            s = ["digraph prov {"]
        for v, edges in g.items():
            s.extend(["\t%d -> %d [label=%d];" % \
                    (self.rankings[v], self.rankings[e.dest], self.ids[e.label])
                    for e in edges])
        s.append("}")
        return "\n".join(s)

    def get_graph(self, transpose=False):
        if transpose:
            if not self.transpose:
                self.transpose = util.transpose_graph(self.g)
            return self.transpose
        return self.g

    def get_metadata(self):
        return self.metadata

    def construct_identifier_ids(self):
        '''
        Adds the identifiers for relations to the identifier-to-id dictionary and 
        writes this to file.
        The first 32 bits of the file represent the number of nodes.
        '''
        if self.ids:
            return self.ids
        if not self.rankings:
            self.rank()
        node_bits = util.nbits_for_int(len(self.rankings))
        self.ids = self.rankings.copy()
        
        for identifier, metadata in self.metadata.items():
            if metadata.typ == 'relation':
                # put IDs for relation identifiers in the dictionary
                self.ids[identifier] = ((self.rankings[metadata.data['cf:sender']] << node_bits) 
                                        + self.rankings[metadata.data['cf:receiver']])
        sorted_idents = sorted(self.ids.keys(), key=lambda v: self.ids[v])
        
        # reference encoding
        wbs = util.WriterBitString()
        default = sorted_idents[0]
        assert(util.nbits_for_int(len(default)) < 8)
        
        wbs.write_int(len(self.rankings), 32)
        wbs.write_int(len(default), 32)
        byts = wbs.to_bytearray()
        byts += default.encode()
        for ident in sorted_idents:
            num_diffs = 0
            diff_wbs = util.WriterBitString()
            for i,ch in enumerate(ident):
                if ch != default[i]:
                    diff_wbs.write_int(i, 8)
                    diff_wbs.write_int(ord(ch), 8)
                    num_diffs += 1
            len_wbs = util.WriterBitString()
            len_wbs.write_int(num_diffs, 8)
            
            byts += len_wbs.to_bytearray() + diff_wbs.to_bytearray()

        with open("identifiers.txt", 'wb') as f:
            f.write(byts)

        return self.ids
    
    @abc.abstractmethod
    def rank(self):
        pass

# Ranks the nodes using vanilla BFS, visiting the sources in the order given
# by the decreasing "reachable order."
class BfsPreprocessor(CompressionPreprocessor):

    def rank(self):
        #assert(not self.cyclic())
        if self.rankings:
            return self.rankings
        self.rankings = {}
        visited = set() 
        queue = collections.deque()
        counter = 0
        ids = {}
        for node in self.get_reachable_order():
            if node not in visited:
                visited.add(node)
                queue.append(node)
            while queue:
                v = queue.popleft()
                ids[v] = counter
                counter += 1
                for edge in self.g[v]:
                    e = edge.dest
                    if e not in visited:
                        visited.add(e)
                        queue.append(e)
        self.rankings = ids
        return ids

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
        gr = {0:[1], 1:[2,3], 2:[4], 3:[], 4:[], 5:[2], 6:[7], 7:[]}
        gr = {v:[pj.Edge(e, None) for e in edges] for v, edges in gr.items()}
        metadata = {}
    else:
        gr, metadata = pj.json_to_graph_data(sys.argv[1])
    """
    transpose = util.transpose_graph(gr)
    for name1, g in [("graph", gr), ("transpose", transpose)]:
        print("<<< " + name1 + " >>>")
        r = BfsPreprocessor(g, metadata)
        for name2, d in [("deltas", r.get_deltas()),
                ("degrees", r.get_degrees())]:
            print(name2 + ":", d)
            print("mean:", statistics.mean(d))
            print("stdev:", statistics.stdev(d))
            print("median:", statistics.median(d))
            print("mode:", statistics.mode(d))
            print()
        print()
    """
    pp = BfsPreprocessor(gr, metadata)
    for t in [False, True]:
        #print(pp.to_dot(transpose=t))
        print(pp.get_degrees(transpose=t))
        print(pp.get_deltas(transpose=t))
        print()

if __name__ == "__main__":
    main()
