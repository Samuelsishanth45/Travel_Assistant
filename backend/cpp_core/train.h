// train.h
#ifndef TRAIN_H
#define TRAIN_H
#include <vector>
#include <string>
using namespace std;
struct Train {
    std::string trainNumber;
    std::string trainName;

    // station names
    std::string source;
    std::string destination;
std::string departure;
  std::string arrival;
  /*
     vector<string> stations;        // intermediate stations
    vector<string> arrivalTimes;    // arrival at each station
    vector<string> departureTimes;  // departure at each station
    vector<int> distances;  // cumulative distance
*/
    std::vector<std::string> stations;
std::vector<std::string> arrivalTimes;
std::vector<std::string> departureTimes;
std::vector<int> distances;

    // departure/arrival as human-time strings (e.g., "06:10")

    // duration in minutes (primary canonical field)
    // Some files expect duration (minutes), some expect durationMin.
    int duration = 0;        // minutes
    int durationMin = 0;     // alias for compatibility

    // distance in km (primary)
    double distance = 0.0;   // kilometers
    double distanceKm = 0.0; // alias for compatibility

    // optional metadata
    std::string zone;
    std::string type;

    // Helper to keep both fields consistent (callers can still set either)
    void normalize() {
        if (durationMin && !duration) duration = durationMin;
        if (duration && !durationMin) durationMin = duration;
        if (distanceKm && !distance) distance = distanceKm;
        if (distance && !distanceKm) distanceKm = distance;
    }
};

#endif // TRAIN_H

