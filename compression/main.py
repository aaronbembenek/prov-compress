#!/usr/bin/python3
import process_json as pj
import compress_graph
from compress_metadata import Encoder, CompressionEncoder
from preprocess import BfsPreprocessor
import sys

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
    print("Using infile %s\nUsing outfile %s" % (infile, outfile))
    r = BfsPreprocessor(graph, metadata)
    iti = r.construct_identifier_ids()
    e = CompressionEncoder(graph, metadata, iti)
    e.compress_metadata(outfile)
    c = compress_graph.BasicCompressor(r)
    c.compress()
    c.write_to_file(graph_out)

if __name__ == "__main__":
    main()
