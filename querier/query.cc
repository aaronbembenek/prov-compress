#include "queriers.hh"
#include "clp.h"

bool compressed = false;
string metafile = "../compression/compressed_metadata.txt";
string graphfile = "../compression/graph.cpg";
string auditfile = "/tmp/audit.log";

enum {
    opt_compressed = 1, 
    opt_metafile,
    opt_graphfile,
    opt_auditfile,
    opt_help,
};
static const Clp_Option options[] = {
  { "help", 'h', opt_help, 0 , 0},
  { "compressed", 'c', opt_compressed, 0 , Clp_Optional },
  { "cmetafile", 0, opt_metafile, Clp_ValString, Clp_Optional },
  { "cgraphfile", 0, opt_graphfile, Clp_ValString, Clp_Optional },
  { "auditfile", 0, opt_auditfile, Clp_ValString, Clp_Optional },
};

static void help() {
  printf("Usage: ./query [OPTIONS]\n\
Options:\n\
 -h, --help\n\
 -c, --compressed, run queries on compressed data\n\
 --cmetafile=metafile (default: %s)\n\
 --cgraphfile=graphfile (default: %s)\n\
 --auditfile=auditfile(default: %s)\n",
    metafile.c_str(), graphfile.c_str(), auditfile.c_str());
  exit(1);
}


int main(int argc, char *argv[]) {
    Clp_Parser *clp = Clp_NewParser(argc, argv, arraysize(options), options);

    if (argc <= 1) {
        cout << "Running on uncompressed meta/graph data" << endl;
    }

    int opt;
    while ((opt = Clp_Next(clp)) != Clp_Done) {
    switch (opt) {
    case opt_help:
        help();
        break;
    case opt_compressed:
        compressed = true;
        break;
    case opt_metafile:
        metafile = clp->val.s;
        break;
    case opt_graphfile:
        graphfile = clp->val.s;
        break;
    case opt_auditfile:
        auditfile = clp->val.s;
        break;
    default:
        help();
    }
    }
    Clp_DeleteParser(clp);
    
    string dummy_id = "dummy";
    if (compressed) {
        CompressedQuerier q1(metafile, graphfile);
        // test on all identifiers
        for (auto id : q1.get_ids()) {
            auto metadata = q1.get_metadata(id);
            print_dict(metadata);
        }
        for (unsigned i = 0; i < 100; ++i) {
            auto metadata = q1.get_metadata(dummy_id);
            print_dict(metadata);
        }
    } else {
        DummyQuerier q2(auditfile);
         for (auto id : q2.get_ids()) {
            auto metadata = q2.get_metadata(id);
            print_dict(metadata);
        }
        for (unsigned i = 0; i < 100; ++i) {
            auto metadata = q2.get_metadata(dummy_id);
            print_dict(metadata);
        }
    }
    return 0;
}
