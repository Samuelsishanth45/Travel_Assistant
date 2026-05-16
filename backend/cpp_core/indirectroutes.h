#ifndef INDIRECT_ROUTES_H
#define INDIRECT_ROUTES_H

#include <string>


void handleIndirectRoutes(const std::string& src,
                          const std::string& dest);

#endif
 
 
 
 
 
 
 
 
 
 
 /*
 #ifndef INDIRECTROUTES_H
#define INDIRECTROUTES_H

#include <vector>
#include <set>
#include <string>

#include "train.h"

using std::string;
using std::vector;
using std::set;


struct RouteState {
    string currentStation;
    vector<Train> path;
    int arrivalTime;
    set<string> visited;
};


/**
 * Handles indirect train routes using
 * Floyd–Warshall station paths.
 *
 * This function:
 *  - Calls Floyd–Warshall
 *  - Builds real train routes
 *  - Calculates real layovers
 *  - Sorts and displays top 5 routes
 /////
void handleIndirectRoutes(
    const string& source,
    const string& destination
);

vector<vector<Train>> findIndirectRoutes(
    const string &src,
    const string &dest
);


#endif
*/