#ifndef STATION_UTILS_H
#define STATION_UTILS_H

#include <string>
#include <vector>
#include <map>
#include "train.h"

struct Neighbor {
    std::string to;
    unsigned long weight;
};

struct GraphNode {
    std::string name;
    std::vector<std::pair<std::string, unsigned long>> neighbors;
    GraphNode(const std::string& n = "") : name(n) {}
};

std::string formatStationName(const std::string& stationName);
bool isStationMatch(const std::string& input, const std::string& station);
std::vector<std::string> getAllStations(const std::vector<Train>& trains);


std::map<std::string, GraphNode*> buildGraph(const std::vector<Train>& trains);

//std::string cleanTrainName(const std::string& name);
string cleanTrainName(string name);

std::string normalizeStationName(std::string s);


#endif // STATION_UTILS_H






/*
#ifndef STATION_UTILS_H
#define STATION_UTILS_H

#include <string>
#include <vector>
#include <map>

// Forward declarations
struct Train;
struct GraphNode;

std::string formatStationName(const std::string& stationName);
bool isStationMatch(const std::string& input, const std::string& station);
std::vector<std::string> getAllStations(const std::vector<Train>& trains);
std::map<std::string, GraphNode*> buildGraph(const std::vector<Train>& trains);
void cleanupGraph(std::map<std::string, GraphNode*>& graph);

#endif
*/