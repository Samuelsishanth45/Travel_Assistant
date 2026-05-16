#include "train_matrix.h"
#include <unordered_map>
#include <string>

using namespace std;

/* ---- GLOBAL DEFINITIONS (ONLY ONCE) ---- */

unordered_map<string, int> stationUsageCount;

unordered_map<string, string> stationIndex;
unordered_map<string, string> stationNameToCode;
unordered_map<string, string> codeToStationName;

unordered_map<string, vector<string>> cityToStations;

/* ---- MAIN TRAIN MATRIX (OPTION 1 CORE) ---- */

unordered_map<string,
    unordered_map<string, vector<Train>>> trainMatrix;














/*
#include "train_matrix.h"
#include <unordered_map>
#include <string>

std::unordered_map<std::string, int> stationUsageCount;

unordered_map<string, string> stationIndex;
unordered_map<string, string> stationNameToCode;
unordered_map<string, string> codeToStationName;

unordered_map<string, vector<string>> cityToStations;

unordered_map<string, vector<string>> cityToStations;
unordered_map<string, int> stationUsageCount;


//unordered_map<string, int> stationUsageCount;

//const std::vector<Train>& getAllTrains() {
  //  return allTrains;   // use your existing global/static train vector}

// Matrix definition
std::unordered_map<std::string,
    std::unordered_map<std::string, std::vector<Train>>> trainMatrix;

// 🔥 SINGLE definition (ONLY HERE)
//std::unordered_map<std::string, std::string> stationNameToCode;
*/