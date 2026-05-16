#ifndef DIRECT_TRAINS_H
#define DIRECT_TRAINS_H

#include <vector>
#include <string>
#include "train.h"

// === FAST ADDITION FOR DISPLAY FROM main.cpp ===
//string computeDuration(const string& dep, const string& arr);
string computeDuration(const string& dep, const string& arr, int distance); 
vector<string> getSmartIntermediate(const string& src, const string& dest);

std::vector<Train> findDirectTrains(
    const std::string& source,
    const std::string& destination
);

/*
std::vector<Train> findDirectTrains(
    const std::vector<Train>& trains,
    const std::string& source,
    const std::string& destination
);
*/

void displayDirectTrainsBeautiful(
    const std::vector<Train>& trains,
    const std::string& src,
    const std::string& dest
);

#endif




/*#ifndef DIRECT_TRAINS_H
#define DIRECT_TRAINS_H

#include <vector>
#include <string>

// Forward declaration
struct Train;

std::vector<Train> findDirectTrains(const std::vector<Train>& trains, 
                                   const std::string& source, 
                                   const std::string& destination);

std::vector<Train> findBestDirectTrains(const std::string& source, const std::string& destination);
void displayDirectTrainsBeautiful(const std::vector<Train>& trains, 
                                 const std::string& source, 
                                 const std::string& destination);

#endif*/