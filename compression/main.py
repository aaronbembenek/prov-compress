#!/usr/bin/python3
from compress_graph_v2 import GraphCompressorV2
from compress_metadata import CompressionEncoder
from preprocess_v2 import clean_camflow_json, PreprocessorV2
import process_json as pj
import sys
import time
from process_json import graph_to_dot4

def main():
    if len(sys.argv) == 1:
        infile = "/tmp/audit.log"
        outfile = "compressed_metadata.txt"
        graph_out = "graph"
    elif len(sys.argv) == 2:
        infile = sys.argv[1]
        outfile = "compressed_metadata.txt"
        graph_out = "graph"
    elif len(sys.argv) == 3:
        infile = sys.argv[1]
        outfile = sys.argv[2]
        graph_out = "graph"
    else:
        print("Usage: ./process_json.py [infile] [outfile]")
        sys.exit(1)

    with open(infile) as f:
        pp = PreprocessorV2(clean_camflow_json(f))
    pp.process()

    start = time.process_time()
    e = CompressionEncoder(pp)
    e.compress_metadata()
    c = GraphCompressorV2(pp)
    c.compress()
    end = time.process_time()

    e.write_to_file(outfile)
    # Continue to use old extension, to make life easier (maybe).
    c.write_to_file(graph_out, ext="cpg")
    print(graph_to_dot4(pp))

    #print("Compression Time: ", end-start)

if __name__ == "__main__":
    main()
