#include "helpers.hh"
#include "queriers.hh"

string metafile = "../compression/compressed_metadata.txt";
string graphfile = "../compression/graph.cpg";
string auditfile = "../copythrice.log";

int main() {
    CompressedQuerier q(metafile, graphfile);
    DummyQuerier dq(auditfile);
    // first10.txt
    //string file = "AAAIAAAAACBmYAEAAAAAAMVT1VmFSQxzAAAAAAAAAAA=";
    // first10.copy.txt
    string file = "AAAIAAAAACBoYAEAAAAAAMVT1VmFSQxzAAAAAAAAAAA=";
    string task = "AQAAAAAAAEBdYAEAAAAAAMVT1VmFSQxzDQAAAAAAAAA=";
    for (auto p : q.friends_of(file, task)) {
        cout << p.first << endl;
        for (auto id : p.second) {
            cout << id << endl;
        }
    }
    for (auto p : dq.friends_of(file, task)) {
        cout << p.first << endl;
        for (auto id : p.second) {
            cout << id << endl;
        }
    }
}
