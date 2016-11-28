#!/usr/bin/python3
import process_json as pj
from compress_metadata import Encoder, CompressionEncoder
from preprocess import BfsPreprocessor
import sys

def main():
    if len(sys.argv) == 1:
        print("No input file: defaulting to /tmp/audit.log.")
        infile = "/tmp/audit.log"
        print("No output file: defaulting to compressed_metadata.txt.")
        outfile = "compressed_metadata.txt"
    elif len(sys.argv) == 2:
        infile = sys.argv[1]
        print("No output file: defaulting to compressed_metadata.txt.")
        outfile = "compressed_metadata.txt"
    elif len(sys.argv) == 3:
        infile = sys.argv[1]
        outfile = sys.argv[2]
    else:
        print("Usage: ./process_json.py [infile] [outfile]")
        sys.exit(1)

    graph, metadata = pj.json_to_graph_data(infile)
    r = BfsPreprocessor(graph, metadata)
    r.rank()
    #iti = r.construct_identifier_ids()
    e = CompressionEncoder(graph, metadata, iti)
    e.compress_metadata(outfile)

if __name__ == "__main__":
    main()
