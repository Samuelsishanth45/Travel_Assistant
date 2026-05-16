// station_utils.h
#ifndef STATION_MAPPER_H
#define STATION_MAPPER_H

// Standard includes needed by anything that includes this header
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
using std::string;
string getStationCity(const string &stationCode);

/// Helper to clean Google place names for display (remove "Railway Station", "Junction", etc.)
std::string extractProperStationName(const std::string& googleName);

/// Return a cleaned, human-friendly station/place name (DO NOT return station codes)
std::string resolveStationName(const std::string& input);

/// Load the station database (station-name -> station-code).
/// Use std::unordered_map so the declaration is explicit and parsable.
std::unordered_map<std::string, std::string> loadStationDatabase();

/// Return similar station names (readable) for suggestions
//std::vector<std::string> findSimilarStations(const std::string& input);

/// If you need a station code explicitly, this helper returns it (may return empty string).
std::string findStationCode(const std::string& stationName);

std::vector<std::string> findSimilarStations(const std::string& input);


std::string formatStationName(const std::string& stationName);
bool isStationMatch(const std::string& input, const std::string& station);


std::string resolveCityToBestStation(const std::string& input);

#endif // STATION_MAPPER_H




/*
#ifndef STATION_UTILS_H
#define STATION_UTILS_H
#include <unordered_map>
#include <string>
#include <vector>
#include <map>


// Forward declarations
struct Train;
struct GraphNode;


// ADD THESE DECLARATIONS

std::string resolveStationName(const std::string& input);
std::vector<std::string> findSimilarStations(const std::string& input);
std::string findStationCode(const std::string& stationName);

unordered_map<string, string> loadStationDatabase();

//std::unordered_map<std::string, std::string> loadStationDatabase();


std::string formatStationName(const std::string& stationName);
bool isStationMatch(const std::string& input, const std::string& station);
std::vector<std::string> getAllStations(const std::vector<Train>& trains);
std::map<std::string, GraphNode*> buildGraph(const std::vector<Train>& trains);
void cleanupGraph(std::map<std::string, GraphNode*>& graph);

#endif
*/