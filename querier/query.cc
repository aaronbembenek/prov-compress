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
    vector<string> str_vector;
    if (compressed) {
        CompressedQuerier q1(metafile, graphfile);
        // test on all identifiers
        for (auto id : q1.get_node_ids()) {
            auto metadata = q1.get_metadata(id);
            print_str_vector(q1.get_all_ancestors(id));
            print_str_vector(q1.get_direct_ancestors(id));
            print_str_vector(q1.get_all_descendants(id));
            print_str_vector(q1.get_direct_descendants(id));
            (q1.all_paths(id, id));
            (q1.friends_of(id));
        }
        for (unsigned i = 0; i < 100; ++i) {
            auto metadata = q1.get_metadata(dummy_id);
            print_str_vector(q1.get_all_ancestors(dummy_id));
            print_str_vector(q1.get_direct_ancestors(dummy_id));
            print_str_vector(q1.get_all_descendants(dummy_id));
            print_str_vector(q1.get_direct_descendants(dummy_id));
            (q1.all_paths(dummy_id, dummy_id));
            (q1.friends_of(dummy_id));
        }
    } else {
        DummyQuerier q2(auditfile);
        for (auto id : q2.get_node_ids()) {
            auto metadata = q2.get_metadata(id);
            cout << "\nNEW ID " << id << endl;
            cout << metadata["cf:id"] << endl;
            //str_vector = q2.get_all_ancestors(id);
            //str_vector = (q2.get_direct_ancestors(id));
            //str_vector = (q2.get_all_descendants(id));
            //str_vector = (q2.get_direct_descendants(id));
            //(q2.friends_of(id));
            string s1 = "AQAAAAAAAECT0wEAAAAAADsFMvWRTtonAQAAAAAAAAA=";
            string s2 = "AAAIAAAAACCR0wEAAAAAADsFMvWRTtonAAAAAAAAAAA=";
            vector<vector<string>> str_vectors = q2.all_paths(s2, s1);
            for (auto sv : str_vectors) {
                print_str_vector(sv);
            }
            //print_str_vector(str_vector);
        }
        for (unsigned i = 0; i < 100; ++i) {
            /*
            auto metadata = q2.get_metadata(dummy_id);
            (q2.get_all_ancestors(dummy_id));
            (q2.get_direct_ancestors(dummy_id));
            (q2.get_all_descendants(dummy_id));
            (q2.get_direct_descendants(dummy_id));
            (q2.all_paths(dummy_id, dummy_id));
            (q2.friends_of(dummy_id));*/
        }
    }
    return 0;
}
