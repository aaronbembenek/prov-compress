#include "query.hh"

class Querier {
public:
   virtual map<string, string> get_metadata(string& identifier) = 0;
   // TODO change signatures to return graph
   virtual void get_all_ancestors(string& identifier) = 0;
   virtual void get_direct_ancestors(string& identifier) = 0;
   virtual void get_all_descendants(string& identifier) = 0;
   virtual void get_direct_descendants(string& identifier) = 0;
   virtual void all_paths(string& sourceid, string& sinkid) = 0;
   virtual void friends_of(string& identifier) = 0;
};

// rename this...
class CompressedQuerier : Querier {
public:
    CompressedQuerier(string& metafile, string& graphfile) : metadata_(metafile) {
        (void)(graphfile);
    }

    map<string, string> get_metadata(string& identifier) override {
        return metadata_.get_metadata(identifier);
    }
    // TODO change signatures to return graph
    void get_all_ancestors(string& identifier) override {
        (void)identifier;
    }
    void get_direct_ancestors(string& identifier) override {
        (void)identifier;
    }
    void get_all_descendants(string& identifier) override {
        (void)identifier;
    }
    void get_direct_descendants(string& identifier) override {
        (void)identifier;
    }
    void all_paths(string& sourceid, string& sinkid) override {
        (void)sourceid;
        (void)sinkid;
    }
    void friends_of(string& identifier) override {
        (void)identifier;
    }
    
private:
    Metadata metadata_;
};

int main(int argc, char *argv[]) {
    string infile;

    if (argc == 1) {
        infile = "compressed_metadata.txt";
    } else if (argc == 2) {
        infile = argv[1];
    } else {
        cout << "Usage: ./query [infile]" << endl;
        return 1;
    }

    Metadata m(infile);
    vector<string> identifiers = {
        "cf:AgAAAACI//9rAAAAAAAAABMiaFq3/swrAAAAAAAAAAA=",
        "cf:BAAAAAAAAAAj+wYAAAAAABMiaFq3/swrAQAAAAAAAAA=", 
        "dummy"
    };
    for (auto id : identifiers) {
        auto metadata = m.get_metadata(id);
        print_dict(metadata);
    }
    return 0;
}
