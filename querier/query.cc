#include "helpers.hh"
#include "queriers.hh"
#include "clp.h"

string metafile = "../compression/compressed_metadata.txt";
string graphfile = "../compression/graph.cpg";
string auditfile = "/tmp/audit.log";

enum {
    opt_metafile = 1,
    opt_graphfile,
    opt_auditfile,
    opt_help,
};
static const Clp_Option options[] = {
  { "help", 'h', opt_help, 0 , 0},
  { "cmetafile", 0, opt_metafile, Clp_ValString, Clp_Optional },
  { "cgraphfile", 0, opt_graphfile, Clp_ValString, Clp_Optional },
  { "auditfile", 0, opt_auditfile, Clp_ValString, Clp_Optional },
};

static void help() {
  printf("Usage: ./query [OPTIONS]\n\
Options:\n\
 -h, --help\n\
 --cmetafile=metafile (default: %s)\n\
 --cgraphfile=graphfile (default: %s)\n\
 --auditfile=auditfile(default: %s)\n",
    metafile.c_str(), graphfile.c_str(), auditfile.c_str());
  exit(1);
}

int main(int argc, char *argv[]) {
    Clp_Parser *clp = Clp_NewParser(argc, argv, arraysize(options), options);


    int opt;
    while ((opt = Clp_Next(clp)) != Clp_Done) {
    switch (opt) {
    case opt_help:
        help();
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
    
    printf("\
        Using Compressed Metadata %s\n\
        Using Compressed Graph %s\n\
        Using Auditfile %s\n\
        Benchmarking %d reps of each query\n",
        metafile.c_str(), graphfile.c_str(), auditfile.c_str(), NUM_REPS);
    
    CompressedQuerier q1(metafile, graphfile);
    DummyQuerier q2(auditfile);
    vector<Querier> qs = {q1, q2};
    for (auto q : qs) {
        cout << "New Querier" << endl;
        cout << "ID, Get_Metadata, Get_All_Ancestors, Get_Direct_Ancestors, Get_All_Descendants, Friends_Of, All_Paths" << endl;
        for (auto id : q.get_node_ids()) {
            cout << id;
            vector<exec_stats<chrono::nanoseconds>> stats;
            stats.push_back(measure<>::execution(q, &Querier::get_metadata, id));
            stats.push_back(measure<>::execution(q, &Querier::get_all_ancestors, id));
            stats.push_back(measure<>::execution(q, &Querier::get_direct_ancestors, id));
            stats.push_back(measure<>::execution(q, &Querier::get_all_descendants, id));
            stats.push_back(measure<>::execution(q, &Querier::get_direct_descendants, id));
            stats.push_back(measure<>::execution(q, &Querier::friends_of, id));

            // run all-paths
            auto startvm = virtualmem_usage();
            auto start = std::chrono::steady_clock::now();
            for (auto i = 0; i < NUM_REPS; ++i) {
                for (auto id2 : q.get_node_ids()) {
                    auto str_vectors = (q.all_paths(id, id)); (void)str_vectors;
                }
            }
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds> (std::chrono::steady_clock::now() - start);
            auto endvm = virtualmem_usage();
           
            for (auto st : stats) {
                cout << ", " << st.time;
            }
            cout << ", " << duration.count() << endl;

            for (auto st : stats) {
                cout << ", " << st.vm_usage;
            }
            cout << ", " << endvm-startvm << endl;
        }
    }
    return 0;
}
