#!/usr/bin/python3

import abc
import collections
import process_json as pj

# Abstract class for rankers.
class Ranker(metaclass=abc.ABCMeta):

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
    
    @abc.abstractmethod
    def rank(self):
        pass

# Ranks the nodes using vanilla BFS, visiting the sources in the order given
# by the decreasing "reachable order."
class BfsRanker(Ranker):

    def __init__(self, graph):
        self.g = graph
        self.rankings = None

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
        return ids

def main():
    #         4
    #         |
    #         2   3   7
    #        / \ /    |
    #       5   1     6
    #           |
    #           0
    g = {0:[1], 1:[2,3], 2:[4], 3:[], 4:[], 5:[2], 6:[7], 7:[]}
    g = {v:[pj.Edge(e, None) for e in edges] for v, edges in g.items()}
    print(BfsRanker(g).rank())

if __name__ == "__main__":
    main()
