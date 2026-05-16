#include "csv_parser.h"
#include <jsoncpp/json/json.h>
#include <fstream>
#include <iostream>
#include "train_matrix.h"
#include <sstream>
#include <unordered_map>
#include <algorithm>   
#include <cctype>     


using namespace std;
/*
vector<Train> loadTrainsFromCSV(const string& filename) {
    // Your CSV file is empty → so return empty.
    return {};
}
*/
int safeStoi(const string& s) {
    try {
        if (s.empty()) return 0;
        return stoi(s);
    } catch (...) {
        return 0;
    }
}


vector<Train> loadTrainsFromCSV(const string& filename) {

    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "❌ ERROR: Cannot open trains.csv: " << filename << endl;
        return {};
    }

    string line;
    getline(file, line); // skip header

    // TEMP structure: TrainNo → Train
    unordered_map<string, Train> tempTrains;

    while (getline(file, line)) {
        stringstream ss(line);
        vector<string> cols;
        string token;

        while (getline(ss, token, ',')) {
            cols.push_back(token);
        }

        if (cols.size() < 12) continue;

        string trainNo      = cols[0];
        string trainName    = cols[1];
      //  int seq             = stoi(cols[2]);
        int seq      = safeStoi(cols[2]);

        string stationCode  = cols[3];
        string stationName  = cols[4];

string name = stationName;
transform(name.begin(), name.end(), name.begin(), ::toupper);

// remove junk words
vector<string> junk = {
    " RAILWAY STATION", " JUNCTION", " JN", " STATION"
};
for (auto &j : junk) {
    size_t pos;
    while ((pos = name.find(j)) != string::npos)
        name.erase(pos, j.length());
}

// trim
while (!name.empty() && name.front() == ' ') name.erase(name.begin());
while (!name.empty() && name.back() == ' ') name.pop_back();

if (!stationCode.empty() && stationCode != "NA") {
    stationNameToCode[name] = stationCode;      // FULL NAME
    stationNameToCode[stationCode] = stationCode; // CODE
}



        /*
      string name = stationName;
transform(name.begin(), name.end(), name.begin(), ::toupper);

// remove suffixes
vector<string> junk = {
    " RAILWAY STATION", " JUNCTION", " JN", " STATION"
};
for (auto &j : junk) {
    size_t pos;
    while ((pos = name.find(j)) != string::npos)
        name.erase(pos, j.length());
}

// trim
while (!name.empty() && name.front() == ' ') name.erase(name.begin());
while (!name.empty() && name.back() == ' ') name.pop_back();

if (!stationCode.empty() && stationCode != "NA") {
    stationNameToCode[name] = stationCode;
    stationNameToCode[stationCode] = stationCode;
}


// ALSO MAP LOWER CONFIDENCE CITY ALIAS
if (!name.empty()) {
    stationNameToCode[name] = stationCode;
}

// city name → station code list
if (!name.empty() && !stationCode.empty() && stationCode != "NA") {
    cityToStations[name].push_back(stationCode);
}

  // 🔥 FIRST WORD ALIAS (CITY SUPPORT)
    // MADGAON JUNCTION → MADGAON
    

// 🔥 FIRST WORD ALIAS (CITY SUPPORT)
size_t sp = name.find(' ');
if (sp != string::npos) {
    string city = name.substr(0, sp);
    stationNameToCode[city] = stationCode;
}
    */

//if (stationCode == "MAO") {
  //  cout << "DEBUG MAP: MADGAON -> " << stationNameToCode["MADGAON"] << endl;
//}

        string arrTime      = cols[5];
        string depTime      = cols[6];
   //     int distance        = stoi(cols[7]);
        int distance = safeStoi(cols[7]);
        string srcCode      = cols[8];
        string srcName      = cols[9];
        string destCode     = cols[10];
        string destName     = cols[11];

        // Create train if first time
        if (tempTrains.find(trainNo) == tempTrains.end()) {
            Train t;
            t.trainNumber = trainNo;
            t.trainName   = trainName;
            t.source      = srcCode;   // CODE ONLY
            t.destination = destCode;  // CODE ONLY
            tempTrains[trainNo] = t;

            stationUsageCount[t.source]++;
stationUsageCount[t.destination]++;
        }

        // Append stop details
        Train &t = tempTrains[trainNo];
        t.stations.push_back(stationCode);
        t.arrivalTimes.push_back(arrTime);
        t.departureTimes.push_back(depTime);
        t.distances.push_back(distance);
    }

    // FINAL STEP: move into MATRIX
    for (auto &pair : tempTrains) {
        Train &t = pair.second;
        t.normalize();
        trainMatrix[t.source][t.destination].push_back(t);
    }

    //stationUsageCount[t.source]++;
//stationUsageCount[t.destination]++;


    cout << "✅ CSV Matrix built with "
         << tempTrains.size()
         << " trains." << endl;

    return {};
}

// REAL FINAL PARSER FOR trains.json (GeoJSON format)
vector<Train> loadTrainsFromJSON(const string& filename) {
/*
    vector<Train> trains;

    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "❌ ERROR: Cannot open trains.json: " << filename << endl;
        return trains;
    }

    // Check if empty
    if (file.peek() == EOF) {
        cerr << "❌ ERROR: trains.json is EMPTY." << endl;
        return trains;
    }

    Json::Value root;
    try {
        file >> root;
    } catch (const exception &e) {
        cerr << "❌ JSON PARSE ERROR: " << e.what() << endl;
        return trains;
    }

    // GeoJSON stores in root["features"]
    if (!root.isMember("features")) {
        cerr << "❌ ERROR: trains.json does not contain `features`." << endl;
        return trains;
    }

    const Json::Value &arr = root["features"];

    for (const auto &item : arr) {

        if (!item.isMember("properties")) continue;

        const Json::Value &p = item["properties"];

        Train t;
        t.trainNumber = p.get("number", "").asString();
        t.trainName   = p.get("name", "").asString();
        t.source      = p.get("from_station_name", "").asString();
        t.destination = p.get("to_station_name", "").asString();
       t.departure   = p.get("departure", "").asString();
        t.arrival     = p.get("arrival", "").asString();
        t.duration    = p.get("duration_m", 0).asInt();
        t.distance    = p.get("distance", 0).asInt();
        t.zone        = p.get("zone", "").asString();
        t.type        = p.get("type", "").asString();

        // MINIMUM REQUIRED
        if (t.trainNumber.empty() || t.source.empty() || t.destination.empty()) continue;

        trains.push_back(t);
    }

    cout << "🎯 Loaded " << trains.size() << " trains from GeoJSON!" << endl;
    return trains;
    */
   return{};
}

