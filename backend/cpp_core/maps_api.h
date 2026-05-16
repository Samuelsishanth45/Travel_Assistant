// maps_api.h
#ifndef MAPS_API_H
#define MAPS_API_H

#include <string>
#include <vector>
#include <utility> 

using std::string;
using std::pair;

// Station struct standardized for all modules
struct Station {
    // common names used across the project
    std::string trainName;    // primary name used by many files
    std::string stationName;  // alternate
    std::string name;         // generic

    std::string address;

    // coords
    double latitude = 0.0;
    double longitude = 0.0;

    // distance to user location in kilometers
    double distance = 0.0;
    double distanceKm = 0.0; // alias

    // convenience
    void normalize() {
        if (distance && !distanceKm) distanceKm = distance;
        if (distanceKm && !distance) distance = distanceKm;
        if (trainName.empty()) trainName = stationName.empty() ? name : stationName;
    }
};

struct Location {
    double latitude = 0.0;
    double longitude = 0.0;

};



// Transit route / train info returned from Google directions
struct TrainInfo {
    std::string trainName;
    std::string short_name;
    std::string departureStation;
    std::string arrivalStation;
    std::string departureTime; // human text
    std::string arrivalTime;   // human text
    std::string departureTimeText, arrivalTimeText;
    std::string duration;
    std::string distance;
    std::string vehicleType;
    std::string agency;
    std::string headsign;
    long departureTimeEpoch = 0;
    long arrivalTimeEpoch = 0;
};

// Encapsulated route composed of TrainInfo steps
struct TrainRoute {
    std::vector<TrainInfo> trains;
    std::string distance;
    std::string duration;
    unsigned long duration_seconds = 0;
    bool hasTrain = false;
};

//
// Prototypes used across the project (implemented in maps_api.cpp)
//

// helper to extract numeric distance from google string ("350 km" / "500 m")
double extractDistance(const std::string& distanceStr);

std::string findNearestStation(const std::string& location);
Station getStationDetails(const std::string& stationName);
std::vector<Station> findStationsByLocation(const std::string& location);

std::vector<TrainRoute> findDirections(const std::string& origin,
                                       const std::string& destination,
                                       const std::string& mode = "");

std::vector<TrainRoute> findTrainRoutes(const std::string& origin,
                                        const std::string& destination);

std::vector<TrainRoute> findTransitRoutes(const std::string& origin,
                                          const std::string& destination);

// Google maps directions url helpers
std::string getDirectionsUrl(const std::string &from, const std::string &to, const std::string &mode);
std::string getDirectionsUrl(const std::string &from, const std::string &to);



struct DistanceMatrixResult {
    long durationSec;
    double distanceKm;
    bool ok;
};

DistanceMatrixResult getDistanceMatrix(const string& origin, const string& dest);

int extractMinutes(const string &durationStr);

Location geocodeAddress(const string& address);


#endif // MAPS_API_H







/*
#ifndef MAPS_API_H
#define MAPS_API_H

#include <string>
#include <vector>
using namespace std;

struct Location {
    double latitude;
    double longitude;
    std::string address;
};

struct Station {
    std::string name;
    std::string address;
    double latitude;
    double longitude;
    double distance;
};

struct TrainInfo {
    std::string name;
    std::string departureStation;
    std::string arrivalStation;
    std::string departureTime;
    std::string arrivalTime;
    std::string duration;
    std::string distance;
};

struct TrainRoute {
    std::vector<TrainInfo> trains;
    std::string distance;
    std::string duration;
};

// Function declarations
std::vector<Station> findStationsByLocation(const std::string& location);
std::string getDirectionsUrl(const std::string& origin, const std::string& destination);
std::string getDirectionsUrl(const std::string& origin, const std::string& destination, const std::string& mode);
std::string findNearestStation(const std::string& location);
Station getStationDetails(const std::string& stationName);
// Add these function declarations to maps_api.h
int extractMinutes(const string& duration);
double calculateTransportCost(const string& distanceStr, const string& mode);
string findOptimalHub(const string& source, const string& destination);

// NEW: Google Maps Train Routes
std::vector<TrainRoute> findTrainRoutes(const std::string& origin, const std::string& destination);

#endif
*/