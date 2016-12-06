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
    
    vector<std::chrono::nanoseconds::rep> times;
    int vm_usage = 0;
    auto ids = q.get_node_ids();

    // friends 
    if (query == 5) {
        vector<string> pathname_ids;
        for (auto id : ids) {
            auto md = q.get_metadata(id);
            if (md["cf:type"] == "file_name") {
                pathname_ids.push_back(id);
            }
        }
        for (auto p1 : pathname_ids){
            for (auto p2 : pathname_ids) {
#if COMPRESSED
                // TODO
                times.push_back(measure<>::execution(q, &CompressedQuerier::friends_of, p1, p2));
#else
                times.push_back(measure<>::execution(q, &DummyQuerier::friends_of, p1, p2));
#endif
                vm_usage = max(vm_usage, virtualmem_usage());
            }
        }
    }
    // all_paths
    else if (query == 6) {
        for (unsigned i = 0; i < ids.size(); i+=50) {
            for (unsigned j = 1; j < ids.size(); j+=100) {
                auto start = std::chrono::steady_clock::now();
                for (auto r = 0; r < NUM_REPS; ++r) {
                    q.all_paths(ids[i], ids[j]);
                }
                auto duration = std::chrono::duration_cast<std::chrono::nanoseconds> (std::chrono::steady_clock::now() - start);
                times.push_back(duration.count());
                vm_usage = max(vm_usage, virtualmem_usage());
            }
        }
    }
    // all other queries
    else {
        for (unsigned i = 0; i < ids.size(); i+= 100) {
            switch(query) {
            case (0):
                times.push_back(measure<>::execution(q, &Querier::get_metadata, ids[i]));
                vm_usage = max(vm_usage, virtualmem_usage());
                break;
            case(1): 
                times.push_back(measure<>::execution(q, &Querier::get_all_ancestors, ids[i]));
                vm_usage = max(vm_usage, virtualmem_usage());
                break;
            case(2): 
                times.push_back(measure<>::execution(q, &Querier::get_direct_ancestors, ids[i]));
                vm_usage = max(vm_usage, virtualmem_usage());
                break;
            case(3): 
                times.push_back(measure<>::execution(q, &Querier::get_all_descendants, ids[i]));
                vm_usage = max(vm_usage, virtualmem_usage());
                break;
            case(4): 
                times.push_back(measure<>::execution(q, &Querier::get_direct_descendants, ids[i]));
                vm_usage = max(vm_usage, virtualmem_usage());
                break;
            default: assert(0);
            }
        }
    }

    for (auto t : times) {
        cout << t <<  ", ";
    }
    cout << endl<< "VM:" << vm_usage << endl;

    return 0;
}
