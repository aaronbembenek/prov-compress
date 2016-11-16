#!/usr/bin/python3

import math
import process_json as pj

def transpose_graph(g):
    t = {}
    for node, edges in g.items():
        t.setdefault(node, [])
        for edge in edges:
            t.setdefault(edge.dest, []).append(pj.Edge(node, edge.label))
    return t

def nbits_for_int(i):
    assert i >= 0
    return math.floor(math.log(max(i, 1))/math.log(2)) + 1

class BitString:

    def __init__(self):
        self.byts = bytearray()
        self.pos = -1
        self.len = 0

    def write_bit(self, b):
        assert b == 0 or b == 1
        if self.pos < 0:
            self.byts.append(0)
            self.pos = 7

        self.byts[-1] |= b << self.pos
        self.pos -= 1
        self.len += 1

    # Writes a non-negative integer to the bitstring, with the highest-order
    # bit appearing leftmost.
    def write_int(self, i, width=0):
        assert i >= 0
        k = nbits_for_int(i) - 1
        if width:
            assert width >= k + 1 
            while width > k + 1:
                self.write_bit(0)
                width -= 1
        while k >= 0:
            self.write_bit((i & (1 << k)) >> k)
            k -= 1

    def to_bytearray(self):
        return self.byts

    def __len__(self):
        return self.len

def main():
    bs = BitString()
    byts = bs.to_bytearray()
    bs.write_int(32)
    bs.write_int(3)
    print(byts.hex())
    print(len(bs))
    bs = BitString()
    byts = bs.to_bytearray()
    bs.write_int(3, width=3)
    print(byts.hex())
    print(len(bs))

if __name__ == "__main__":
    main()
