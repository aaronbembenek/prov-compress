#!/usr/bin/python3

import math
import sys
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

def int2bitstr(int_data, num_bits):
    '''
    Converts an integer into a integer represented in num_bits bits.
    Returns a bit string of 0s and 1s
    '''
    assert(int_data == 0 or math.ceil(math.log(int_data, 2)) <= num_bits)
    format_str = '{0:0%db}' % num_bits
    return (format_str.format(int_data))

class ReaderBitString:

    def __init__(self, byts):
        self.byts = byts
        self.index = -1 
        self.pos = -1

    def read_bit(self):
        if self.pos < 0:
            self.index += 1
            self.pos = 7
            assert self.index < len(self.byts)
        b = 1 if self.byts[self.index] & (1 << self.pos) else 0
        self.pos -= 1
        return b

    def read_int(self, width):
        assert width > 0
        i = 0
        while width > 0:
            i <<= 1
            i |= self.read_bit()
            width -= 1
        return i

    def read_int_at_pos(self, pos, width):
        self.index = pos // 8
        self.pos = 7 - (pos % 8)
        assert self.index < len(self.byts)
        return self.read_int(width)

    def read_bit_at_pos(self, pos):
        return self.read_int_at_pos(pos, 1)

class WriterBitString:

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
        return 1

    # Writes a non-negative integer to the bitstring, with the highest-order
    # bit appearing leftmost.
    def write_int(self, i, width=0):
        assert i >= 0
        sz = nbits_for_int(i)
        written = max(sz, width)
        k = sz - 1
        if width:
            assert width >= k + 1 
            while width > k + 1:
                self.write_bit(0)
                width -= 1
        while k >= 0:
            self.write_bit((i & (1 << k)) >> k)
            k -= 1
        return written 

    def to_bytearray(self):
        return self.byts

    def __len__(self):
        return self.len

def warn(*args, **kargs):
    sep = kargs.get("sep", " ") 
    msg = sep.join(str(x) for x in args)
    print("\033[93m" + msg + "\033[0m", **kargs)

def main():
    bs = WriterBitString()
    byts = bs.to_bytearray()
    bs.write_int(32)
    bs.write_int(3)
    print(byts.hex())
    print(len(bs))

    bs = WriterBitString()
    byts = bs.to_bytearray()
    bs.write_int(3, width=3)
    bs.write_int(113, width=7)
    print(byts.hex())
    print(len(bs))

    bs = ReaderBitString(byts)
    print(bs.read_int(3))
    print(bs.read_int(7))

if __name__ == "__main__":
    main()
