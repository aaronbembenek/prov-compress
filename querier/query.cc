#include "helpers.hh"
#include "queriers.hh"
#include "clp.h"

string metafile = "../compression/compressed_metadata.txt";
string graphfile = "../compression/graph.cpg";
string auditfile = "/tmp/audit.log";

enum {
    opt_metafile = 1,
    opt_query,
    opt_graphfile,
    opt_auditfile,
    opt_help,
};
static const Clp_Option options[] = {
  { "help", 'h', opt_help, 0 , 0},
  { "query", 0, opt_query, Clp_ValInt, Clp_Optional },
  { "cmetafile", 0, opt_metafile, Clp_ValString, Clp_Optional },
  { "cgraphfile", 0, opt_graphfile, Clp_ValString, Clp_Optional },
  { "auditfile", 0, opt_auditfile, Clp_ValString, Clp_Optional },
};

static void help() {
  printf("Usage: ./query [OPTIONS]\n\
Options:\n\
 -h, --help\n\
 -c, --compressed\n\
 --query=[0-6] (default: 0)\n\
 --cmetafile=metafile (default: %s)\n\
 --cgraphfile=graphfile (default: %s)\n\
 --auditfile=auditfile(default: %s)\n",
    metafile.c_str(), graphfile.c_str(), auditfile.c_str());
  exit(1);
}

int main(int argc, char *argv[]) {
    int query = 0;

    Clp_Parser *clp = Clp_NewParser(argc, argv, arraysize(options), options);

    int opt;
    while ((opt = Clp_Next(clp)) != Clp_Done) {
    switch (opt) {
    case opt_help:
        help();
        break;
    case opt_query:
        query = (int)clp->val.i;
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
   
#if COMPRESSED
    fprintf(stderr,"\
        Using Compressed Metadata %s\n\
        Using Compressed Graph %s\n\
        %d reps\n",
        metafile.c_str(), graphfile.c_str(), NUM_REPS);
    cout << "Query " << query << endl;
    CompressedQuerier q(metafile, graphfile);
#else
    fprintf(stderr,"\
        Using Auditfile %s\n\
        %d reps\n",
        auditfile.c_str(), NUM_REPS);
    cout << "Query " << query << endl;
    DummyQuerier q(auditfile);
#endif
    
    vector<string> ids;
    vector<std::chrono::nanoseconds::rep> times;
    int vm_usage = 0;
    for (auto id : q.get_node_ids()) {
        ids.push_back(id);
        // run all-paths
        if (query == 6) {
            auto start = std::chrono::steady_clock::now();
            for (auto i = 0; i < NUM_REPS; ++i) {
                for (auto id2 : q.get_node_ids()) {
                    (q.all_paths(id, id));
                }
            }
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds> (std::chrono::steady_clock::now() - start);
            times.push_back(duration.count());
            vm_usage = virtualmem_usage();
        } else {
            switch(query) {
            case (0):
                times.push_back(measure<>::execution(q, &Querier::get_metadata, id));
                vm_usage = virtualmem_usage();
                break;
            case(1): 
                times.push_back(measure<>::execution(q, &Querier::get_all_ancestors, id));
                vm_usage = virtualmem_usage();
                break;
            case(2): 
                times.push_back(measure<>::execution(q, &Querier::get_direct_ancestors, id));
                vm_usage = virtualmem_usage();
                break;
            case(3): 
                times.push_back(measure<>::execution(q, &Querier::get_all_descendants, id));
                vm_usage = virtualmem_usage();
                break;
            case(4): 
                times.push_back(measure<>::execution(q, &Querier::get_direct_descendants, id));
                vm_usage = virtualmem_usage();
                break;
            case(5): 
                times.push_back(measure<>::execution(q, &Querier::friends_of, id));
                vm_usage = virtualmem_usage();
                break;
            default: assert(0);
            }
        }
    }

    /*
    for (auto id : ids) {
        cout << id << ",";
    }
    */
    cout << endl;
    for (auto t : times) {
        cout << t <<  ", ";
    }
    cout << endl<< "VM:" << vm_usage << endl;

    return 0;
}
