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

class Querier;
class CompressedQuerier;

#endif /* QUERY_H */
