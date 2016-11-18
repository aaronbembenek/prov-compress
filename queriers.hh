#ifndef QUERY_H
#define QUERY_H

#include "helpers.hh"
#include "metadata.hh"

/*
    SUPPORTED QUERIES:
    direct_ancestor(identifier) => return identifier
    all_ancestors(identifier) => return list of identifiers
    all_descendants(identifier) => return list of identifiers
    all_paths(source, sink) => return list of list of identifiers
    friends(identifier) => return list of identifiers (is this a useful query to support?)
    metadata(identifier) => return (JSON output?) of identifier
 */

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

class CompressedQuerier : Querier {
public:
    CompressedQuerier(string& metafile, string& graphfile);
    map<string, string> get_metadata(string& identifier) override;
    void get_all_ancestors(string& identifier) override;
    void get_direct_ancestors(string& identifier) override;
    void get_all_descendants(string& identifier) override;
    void get_direct_descendants(string& identifier) override;
    void all_paths(string& sourceid, string& sinkid) override;
    void friends_of(string& identifier) override;
    
private:
    Metadata* metadata_;
};

#endif /* QUERY_H */
