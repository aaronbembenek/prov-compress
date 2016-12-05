class Edge:

    def __init__(self, dest, label):
        self.dest = dest
        self.label = label

    def __repr__(self):
        return '(dest = "%s", label = "%s")' % (self.dest, self.label)


class LabeledBackEdgeGraph:

    def __init__(self):
        self.g = {}

    def __len__(self):
        return len(g)

    def add_vertex(self, node):
        self.g.setdefault(node, ([], []))

    def add_edge(self, source, dest, label):
        self.g[source][0].append(Edge(dest, label))
        self.g[dest][1].append(Edge(source, label))

    def get_outgoing_edges(self, node):
        return self.g[node][0]

    def get_incoming_edges(self, node):
        return self.g[node][1]

    def get_vertices(self):
        return self.g.keys()


class Graph:

    def __init__(self):
        self.g = {}

    def __len__(self):
        return len(g)

    def add_vertex(self, node):
        self.g.setdefault(node, [])

    def add_edge(self, source, dest):
        self.g[source].append(dest)

    def get_outgoing_edges(self, node):
        return self.g[node]

    def get_vertices(self):
        return self.g.keys()
