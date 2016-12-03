#!/usr/bin/python3
import process_json as pj
import compress_graph
from compress_metadata import CompressionEncoder
from preprocess import BfsPreprocessor
import sys
import time

def main():
    if len(sys.argv) == 1:
        infile = pj.PATH+"/tmp/audit.log"
        outfile = pj.PATH+"/compressed_metadata.txt"
        graph_out = pj.PATH+"/graph"
    elif len(sys.argv) == 2:
        infile = sys.argv[1]
        outfile = pj.PATH+"/compressed_metadata.txt"
        graph_out = pj.PATH+"/graph"
    elif len(sys.argv) == 3:
        infile = sys.argv[1]
        outfile = sys.argv[2]
        graph_out = outfile 
    else:
        print("Usage: ./process_json.py [infile] [outfile]")
        sys.exit(1)

    graph, metadata = pj.json_to_graph_data(infile)
    #print("Using infile %s\nUsing outfile %s" % (infile, outfile))
    r = BfsPreprocessor(graph, metadata)

    start = time.process_time()
    iti = r.construct_identifier_ids()
    e = CompressionEncoder(graph, metadata, iti)
    e.compress_metadata()
    c = compress_graph.BasicCompressor(r)
    c.compress()
    end = time.process_time()

    e.write_to_file(outfile)
    c.write_to_file(graph_out)

    print("Compression Time: ", end-start)

if __name__ == "__main__":
    main()
