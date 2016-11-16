#!/usr/bin/python3

import abc
import collections
import process_json as pj
import statistics
import sys
import util

# Abstract class for compression preprocesor.
class CompressionPreprocessor(metaclass=abc.ABCMeta):

    def __init__(self, graph):
        self.g = graph 
        self.rankings = None
        self.deltas = None
        self.degrees = None

    # Return the nodes in decreasing "reachable order." The nodes are
    # ordered so that a node is earlier in the order the more nodes are
    # reachable from it. This is useful for identifying the order in which to
    # visit the sources during a graph search.
    def get_reachable_order(self):
        reachable_size = {}
        def get_reachable_size(v):
            if v in reachable_size:
                return reachable_size[v]
            edges = [e.dest for e in self.g[v]]
            x = len(edges)
            for edge in edges:
                x += get_reachable_size(edge)
            reachable_size[v] = x
            return x
        for node in self.g.keys():
            get_reachable_size(node)
        return sorted(self.g.keys(), key=lambda v: -reachable_size[v])

    def get_deltas(self):
        if not self.rankings:
            self.rank()
        if self.deltas:
            return self.deltas
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
                degrees.append(len(self.g[v]))
                for edge in self.g[v]:
                    e = edge.dest
                    delta = abs(self.rankings[v] - self.rankings[e])
                    deltas.append(delta)
                    if e not in visited:
                        visited.add(e)
                        queue.append(e)
        self.deltas = sorted(deltas)
        self.degrees = sorted(degrees)
        return self.deltas

    def get_degrees(self):
        if not self.degrees:
            self.get_deltas()
        return self.degrees

    def to_dot(self):
        if not self.rankings:
            self.rank()
        s = ["digraph prov {"]
        for v, edges in self.g.items():
            s.extend(["\t%d -> %d;" % \
                    (self.rankings[v], self.rankings[e.dest]) for e in edges])
        s.append("}")
        return "\n".join(s)
    
    @abc.abstractmethod
    def rank(self):
        pass

# Ranks the nodes using vanilla BFS, visiting the sources in the order given
# by the decreasing "reachable order."
class BfsPreprocessor(CompressionPreprocessor):

    def rank(self):
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

def test1():
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
    r = BfsPreprocessor(g)
    print(r.rank())
    print(r.get_deltas())
    print(r.get_degrees())

def main():
    if len(sys.argv) < 2:
        test1()
    else:
        graph, _ = pj.json_to_graph_data(sys.argv[1])
        transpose = util.transpose_graph(graph)
        for name1, g in [("graph", graph), ("transpose", transpose)]:
            print("<<< " + name1 + " >>>")
            r = BfsPreprocessor(g)
            for name2, d in [("deltas", r.get_deltas()),
                    ("degrees", r.get_degrees())]:
                print(name2 + ":", d)
                print("mean:", statistics.mean(d))
                print("stdev:", statistics.stdev(d))
                print("median:", statistics.median(d))
                print("mode:", statistics.mode(d))
                print()
            print()

if __name__ == "__main__":
    main()
