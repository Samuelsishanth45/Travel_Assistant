#include "train.h"
#include "maps_api.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <map>
#include <set>

using namespace std;
/*
vector<Train> loadTrainsFromCSV(const string& filename) {
    vector<Train> trains;
    ifstream file(filename);
    
    if (!file.is_open()) {
        cout << "❌ Could not open CSV file: " << filename << endl;
        cout << "💡 Using Google Maps API for train information" << endl;
        return trains;
    }
    
    string line;
    map<string, Train> trainMap;
    int lineCount = 0;
    int validTrains = 0;
    
    cout << "🚆 Loading train data from CSV..." << endl;
    
    // Skip header if exists
    getline(file, line);
    
    while (getline(file, line)) {
        lineCount++;
        if (line.empty()) continue;
        
        vector<string> tokens;
        string token;
        stringstream ss(line);
        
        // Simple CSV parsing
        while (getline(ss, token, ',')) {
            // Clean the token
            token.erase(remove(token.begin(), token.end(), '\"'), token.end());
            token.erase(remove(token.begin(), token.end(), '\r'), token.end());
            tokens.push_back(token);
        }
        
        if (tokens.size() < 8) continue;
        
        string trainNumber = tokens[0];
        string stationName = tokens[3];
        
        if (trainNumber.empty() || stationName.empty()) continue;
        
        // Clean station name
        stationName = formatStationName(stationName);
        if (stationName.empty()) continue;
        
        // Get other data
        string arrivalTime = (tokens.size() > 5) ? tokens[5] : "00:00";
        string departureTime = (tokens.size() > 6) ? tokens[6] : "00:00";
        
        int distance = 0;
        if (tokens.size() > 7) {
            try {
                distance = stoi(tokens[7]);
            } catch (...) {
                distance = 0;
            }
        }
        
        // Create or update train
        if (trainMap.find(trainNumber) == trainMap.end()) {
            Train newTrain;
            newTrain.trainNumber = trainNumber;
            newTrain.trainName = "Train " + trainNumber;
            trainMap[trainNumber] = newTrain;
        }
        
        trainMap[trainNumber].stations.push_back(stationName);
        trainMap[trainNumber].arrivalTimes.push_back(arrivalTime);
        trainMap[trainNumber].departureTimes.push_back(departureTime);
        trainMap[trainNumber].distances.push_back(distance);
    }
    
    file.close();
    
    // Convert to vector
    for (auto& pair : trainMap) {
        if (pair.second.stations.size() >= 2) {
            trains.push_back(pair.second);
            validTrains++;
        }
    }
    
    cout << "✅ Loaded " << validTrains << " trains from CSV" << endl;
    return trains;
}
*/
vector<pair<string, pair<int, string>>> findDirectTrains(
    const vector<Train>& trains, 
    const string& source, 
    const string& destination) {
    
    vector<pair<string, pair<int, string>>> directTrains;
    
    cout << "\n🎯 Finding trains from " << source << " to " << destination << endl;
    
    // First, try to find stations in our CSV data
    string sourceStation = findBestStationMatch(source);
    string destStation = findBestStationMatch(destination);
    
    cout << "📍 Source: " << sourceStation << endl;
    cout << "🎯 Destination: " << destStation << endl;
    
    // Try CSV data first
    if (!trains.empty()) {
        cout << "🔍 Searching in local database..." << endl;
        
        for (const auto& train : trains) {
            int srcIdx = -1, destIdx = -1;
            
            // Find source and destination in train route
            for (size_t i = 0; i < train.stations.size(); i++) {
                if (isStationMatch(train.stations[i], sourceStation)) {
                    srcIdx = i;
                }
                if (isStationMatch(train.stations[i], destStation)) {
                    destIdx = i;
                }
            }
            
            if (srcIdx != -1 && destIdx != -1 && srcIdx < destIdx) {
                int distance = train.distances[destIdx] - train.distances[srcIdx];
                if (distance < 1) distance = 100; // Default
                
                string depTime = train.departureTimes[srcIdx].substr(0, 5);
                string arrTime = train.arrivalTimes[destIdx].substr(0, 5);
                string travelTime = calculateTravelTime(depTime, arrTime);
                
                string routeInfo = train.stations[srcIdx] + " → " + train.stations[destIdx];
                
                directTrains.push_back({
                    train.trainNumber + " - " + train.trainName, 
                    {distance, depTime + "|" + arrTime + "|" + travelTime + "|" + routeInfo}
                });
                
                // Limit results
                if (directTrains.size() >= 5) break;
            }
        }
    }
    
    // If no trains found in CSV, try Google Maps
    if (directTrains.empty()) {
        cout << "🔍 No local data found. Using Google Maps..." << endl;
        
        auto googleRoutes = findTrainRoutes(source, destination);
        
        for (const auto& route : googleRoutes) {
            for (const auto& train : route.trains) {
                string trainInfo = train.departureTime + "|" + train.arrivalTime + "|" + 
                                 train.duration + "|" + train.departureStation + " → " + train.arrivalStation;
                
                int distance = 100; // Default
                string distStr = train.distance;
                size_t kmPos = distStr.find(" km");
                if (kmPos != string::npos) {
                    distStr = distStr.substr(0, kmPos);
                }
                distStr.erase(remove(distStr.begin(), distStr.end(), ','), distStr.end());
                try {
                    distance = stoi(distStr);
                } catch (...) {
                    distance = 100;
                }
                
                directTrains.push_back({train.trainName, {distance, trainInfo}});
            }
        }
    }
    
    if (directTrains.empty()) {
        cout << "❌ No trains found between these stations" << endl;
        cout << "💡 Try checking station names or nearby cities" << endl;
    } else {
        cout << "✅ Found " << directTrains.size() << " train option(s)" << endl;
    }
    
    return directTrains;
}

string calculateTravelTime(const string& depTime, const string& arrTime) {
    // Google Maps already provides duration, so we don't need to calculate
    return "Check schedule";
}

string formatStationName(const string& stationName) {
    string formatted = stationName;
    transform(formatted.begin(), formatted.end(), formatted.begin(), ::toupper);
    return formatted;
}

vector<string> getAllStations(const vector<Train>& trains) {
    // Return empty since we're using Google Maps
    return {};
}

bool isStationMatch(const string& station, const string& searchTerm) {
    string stationUpper = station;
    string searchUpper = searchTerm;
    transform(stationUpper.begin(), stationUpper.end(), stationUpper.begin(), ::toupper);
    transform(searchUpper.begin(), searchUpper.end(), searchUpper.begin(), ::toupper);
    
    return (stationUpper.find(searchUpper) != string::npos || 
            searchUpper.find(stationUpper) != string::npos);
}

string findBestStationMatch(const string& cityName) {
    // Use Google Maps to find the best station
    return findNearestStation(cityName);
}