#!/usr/bin/python3

class UnionFind:
    def __init__(self, iterable):
        self.elts = {}
        self.sizes = {}
        self.roots = {}
        for x in iterable:
            self.sizes[x] = 1
            self.elts[x] = x
            self.roots[x] = [x]

    def find(self, x):
        return self.elts[x]

    def union(self, x, y):
        x = self.find(x)
        y = self.find(y)
        if x == y:
            return False
        if self.sizes[x] < self.sizes[y]:
            small = x
            big = y
        else:
            small = y
            big = x
        for z in self.roots[small]:
            self.elts[z] = big 
        self.roots[big].extend(self.roots[small])
        del self.roots[small]
        self.sizes[big] += self.sizes[small]
        del self.sizes[small]
        return True

    def leaders(self):
        return list(self.roots.keys())

class FasterUnionFind:
    def __init__(self, iterable):
        self.elts = {}
        self.sizes = {}
        for x in iterable:
            self.sizes[x] = 1
            self.elts[x] = x

    def union(self, x, y):
        x = self.find(x)
        y = self.find(y)
        if x == y:
            return False
        if self.sizes[x] < self.sizes[y]:
            small = x
            big = y
        else:
            small = y
            big = x
        self.elts[small] = big
        self.sizes[big] += self.sizes[small]
        del self.sizes[small]
        return True

    def find(self, x):
        prev = x
        cur = self.elts[x]
        while prev != cur:
            nxt = self.elts[cur]
            self.elts[prev] = nxt
            prev = cur
            cur = nxt 
        return cur

    def leaders(self):
        return list(self.sizes.keys())

class CustomUnionFind(FasterUnionFind):
    def union(self, x, y):
        x = self.find(x)
        y = self.find(y)
        if x == y:
            return False
        self.elts[y] = x
        self.sizes[x] += self.sizes[y]
        del self.sizes[y]
        return True

    def get_size(self, x):
        return self.sizes[self.find(x)]

def main():
    uf = CustomUnionFind([1, 2, 3, 4, 5])
    print(uf.leaders())
    uf.union(1, 2)
    print(uf.leaders())
    uf.union(1, 3)
    print(uf.leaders())
    uf.union(2, 3)
    print(uf.leaders())
    uf.union(4, 5)
    print(uf.leaders())
    uf.union(1, 5)
    print(uf.leaders())
    for i in range(1, 6):
        print(uf.find(i))

if __name__ == "__main__":
    main()
