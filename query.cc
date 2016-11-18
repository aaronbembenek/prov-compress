#include "queriers.hh"

int main(int argc, char *argv[]) {
    string metafile, graphfile;

    if (argc == 1) {
        metafile = "compressed_metadata.txt";
    } else if (argc == 2) {
        metafile = argv[1];
    } else {
        cout << "Usage: ./query [metafile]" << endl;
        return 1;
    }

    CompressedQuerier q(metafile, graphfile);
    vector<string> identifiers = {
        "cf:AgAAAACI//9rAAAAAAAAABMiaFq3/swrAAAAAAAAAAA=",
        "cf:BAAAAAAAAAAj+wYAAAAAABMiaFq3/swrAQAAAAAAAAA=", 
        "dummy"
    };
    for (auto id : identifiers) {
        auto metadata = q.get_metadata(id);
        print_dict(metadata);
    }
    return 0;
}
