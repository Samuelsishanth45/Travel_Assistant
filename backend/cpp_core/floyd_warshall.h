#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "train.h"

using namespace std;
/*
struct RouteState {
    long long totalTime;
    int next;
    vector<Train> chain;
    RouteState() : totalTime(9e18), next(-1) {}
    RouteState(long long t, int n) : totalTime(t), next(n) {}
};
*/
struct RouteState {
    long long totalTime;
    int next;

    RouteState(long long t = 1e18, int n = -1)
        : totalTime(t), next(n) {}
};


extern vector<vector<RouteState>> distMatrix;
//extern vector<string> expandedNodeList;
//extern unordered_map<string,int> expandedNodeIndex;

extern vector<vector<int>> kPaths;

vector<int> getShortestPath(int src, int dest);

extern unordered_map<string,int>stationIndex;

extern unordered_map<string,int> floydstationIndex;

void computeTimeFloydMatrix(const string &src,
                            const string &dest);









/*
#ifndef FLOYD_WARSHALL_H
#define FLOYD_WARSHALL_H

#include <unordered_map>
#include <string>
#include <vector>
using namespace std;

struct FWRoute {
    std::vector<std::string> stations;
};
using FWRouteList = std::vector<FWRoute>;



extern std::unordered_map<
    std::string,
    std::unordered_map<std::string, int>
> minHopMatrix;

extern unordered_map<string,
    unordered_map<string, vector<long long>>> distTime;


    extern unordered_map<string,
    unordered_map<string, vector<string>>> parentNode;


FWRouteList computeFWRoutes(const std::string& src,
                            const std::string& dest,
                            int maxRoutes);

// reachability check (used for pruning)
bool fwReachable(const std::string& u,
                 const std::string& v);

void computeTimeFloydMatrix(const string &src,
                            const string &dest);



void computeMinHopMatrix();

//void computeTimeFloydMatrix();

#endif
*/









/*
#ifndef FLOYD_WARSHALL_H
#define FLOYD_WARSHALL_H

#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include "train.h"

using std::string;
using std::vector;

// One complete journey 
struct FWRoute {
    vector<string> stations;   // MAO → MUM → SUR → DEL
    vector<Train>  trains;     // exact trains used
};

extern std::unordered_map<
    std::string,
    std::unordered_map<std::string, int>
> minHopMatrix;

void computeMinHopMatrix();

// Top routes 
using FWRouteList = vector<FWRoute>;

// Main algorithm 
FWRouteList computeFWRoutes(
    const string& source,
    const string& destination,
    int maxRoutes = 5
);

#endif
*/