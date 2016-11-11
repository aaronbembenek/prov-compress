#!/usr/bin/python3
import process_json as pj
import compress_metadata as cm
import sys

def main():
    if len(sys.argv) == 1:
        print("No input file: defaulting to /tmp/audit.log.")
        infile = "/tmp/audit.log"
    elif len(sys.argv) == 2:
        infile = sys.argv[1]
    elif len(sys.argv) == 3:
        infile = sys.argv[1]
        outfile = sys.argv[2]
    else:
        print("Usage: ./process_json.py [infile] [outfile]")
        sys.exit(1)

    graph, metadata = pj.json_to_graph_data(infile)
    iti = pj.identifier_to_int(graph)

    se = cm.StringEncoder(graph, metadata, iti)
    print(se.compress_metadata())
    #dots_input = graph_to_dot(infile)
    #gspan_input = graph_to_gspan(infile)

if __name__ == "__main__":
    main()
