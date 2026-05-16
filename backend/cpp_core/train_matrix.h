/*
#ifndef TRAIN_MATRIX_H
#define TRAIN_MATRIX_H

#include <unordered_map>
#include <vector>
#include <string>

#include "train.h"

using std::string;

extern std::unordered_map<std::string, std::string> stationNameToCode;


// Sparse matrix:
// source_code -> destination_code -> list of trains
extern std::unordered_map<
    string,
    std::unordered_map<string, std::vector<Train>>
> trainMatrix;

#endif // TRAIN_MATRIX_H
*/
#ifndef TRAIN_MATRIX_H
#define TRAIN_MATRIX_H

#include <unordered_map>
#include <vector>
#include <string>
#include "train.h"
struct Train; // forward declaration

extern std::unordered_map<std::string, int> stationUsageCount;



//const std::vector<Train>& getAllTrains();


extern std::unordered_map<std::string,
    std::unordered_map<std::string, std::vector<Train>>> trainMatrix;

    extern unordered_map<string, vector<string>> cityToStations;

// 🔥 THIS LINE IS CRITICAL
//extern std::unordered_map<std::string, std::string> stationNameToCode;
extern unordered_map<string, string> stationNameToCode;

#endif

