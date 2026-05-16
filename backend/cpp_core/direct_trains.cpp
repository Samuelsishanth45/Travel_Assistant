#include "direct_trains.h"
#include "maps_api.h"
#include "station_mapper.h"
#include "csv_parser.h"
#include "station_utils.h"     // ← MUST BE HERE
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include "train_matrix.h"
//#include "ai_station_resolver.h"
#include <cctype>



using namespace std;

string normalizeStation(string s) {
    // Convert to uppercase
    transform(s.begin(), s.end(), s.begin(), ::toupper);

    // Remove words Google adds
    vector<string> junk = {
        "RAILWAY STATION",
        "JUNCTION",
        "JN",
        "STATION"
    };

    for (auto &j : junk) {
        size_t pos;
        while ((pos = s.find(j)) != string::npos) {
            s.erase(pos, j.length());
        }
    }

    // Trim spaces
    while (!s.empty() && s.front() == ' ') s.erase(s.begin());
    while (!s.empty() && s.back() == ' ') s.pop_back();

    return s;
}

bool looksLikeStationCode(const string& s) {
    if (s.size() < 2 || s.size() > 5) return false;
    for (char c : s) {
        if (!isalpha(c)) return false;
    }
    return true;
}

vector<Train> findDirectTrains(const string& srcInput,
                               const string& dstInput)
{
    string srcN = normalizeStationName(srcInput);
    string dstN = normalizeStationName(dstInput);

    vector<Train> result;

    for (auto &srcPair : trainMatrix) {
        for (auto &dstPair : srcPair.second) {
            for (auto &t : dstPair.second) {

                int srcIdx = -1, dstIdx = -1;
                int srcDist = 0, dstDist = 0;
                string srcDep, dstArr;

                for (size_t i = 0; i < t.stations.size(); i++) {
                    string st = normalizeStationName(t.stations[i]);

                    if (st == srcN && srcIdx == -1) {
                        srcIdx  = i;
                        srcDist = t.distances[i];
                        srcDep  = t.departureTimes[i];
                    }
                    if (st == dstN && srcIdx != -1) {
                        dstIdx  = i;
                        dstDist = t.distances[i];
                        dstArr  = t.arrivalTimes[i];
                        break;
                    }
                }

                if (srcIdx != -1 && dstIdx != -1 && srcIdx < dstIdx) {
                    Train copy = t;

                    copy.source      = t.stations[srcIdx];
                    copy.destination = t.stations[dstIdx];
                    copy.departure   = srcDep;
                    copy.arrival     = dstArr;
                    copy.distance    = dstDist - srcDist;

                    // compute duration
                    int sh = stoi(srcDep.substr(0,2));
                    int sm = stoi(srcDep.substr(3,2));
                    int eh = stoi(dstArr.substr(0,2));
                    int em = stoi(dstArr.substr(3,2));

                    int start = sh*60 + sm;
                    int end   = eh*60 + em;
                    if (end < start) end += 24*60;

                    copy.duration = end - start;

                    result.push_back(copy);
                }
            }
        }
    }

    return result;
}




/*
vector<Train> findDirectTrains(const string& srcInput,
                               const string& dstInput)
{
    // 1️⃣ Resolve USER INPUT → STATION CODES
    string srcCode = findStationCode(srcInput);
    string dstCode = findStationCode(dstInput);

    if (srcCode.empty() || dstCode.empty()) {
        cout << "❌ Couldn't identify station.\n";
        return {};
    }

    vector<Train> result;

    // 2️⃣ Scan ALL trains
    for (auto &srcPair : trainMatrix) {
        for (auto &dstPair : srcPair.second) {
            for (auto &t : dstPair.second) {

                int srcIdx = -1, dstIdx = -1;

                for (size_t i = 0; i < t.stations.size(); i++) {
                    if (t.stations[i] == srcCode) srcIdx = i;
                    if (t.stations[i] == dstCode) dstIdx = i;
                }

                // 3️⃣ Valid direction
                if (srcIdx != -1 && dstIdx != -1 && srcIdx < dstIdx) {
                    Train copy = t;

                    copy.departure = t.departureTimes[srcIdx];
                    copy.arrival   = t.arrivalTimes[dstIdx];
                    copy.distance  = t.distances[dstIdx] - t.distances[srcIdx];

                    // duration from CSV distance/time
                    copy.duration = 0;

                    result.push_back(copy);

                    return result;


                    sort(result.begin(), result.end(),
     [](const Train& a, const Train& b) {
         if (a.duration != b.duration)
             return a.duration < b.duration;   // fastest first
         return a.distance < b.distance;       // then shortest
     });

                }
            }
        }
    }

    return result;
}
*/

string computeDuration(const string& dep, const string& arr, int distance)
{
    if (dep.size() < 5 || arr.size() < 5) return "N/A";

    int dh = stoi(dep.substr(0,2));
    int dm = stoi(dep.substr(3,2));
    int ah = stoi(arr.substr(0,2));
    int am = stoi(arr.substr(3,2));

    int start = dh*60 + dm;
    int end   = ah*60 + am;

    int diff = end - start;

    // ✅ Case 1: normal overnight (23:00 → 05:00)
    if(diff < 0)
        diff += 24*60;

    // ✅ Case 2: YOUR CRITICAL FIX (long-distance but looks same-day)
    // Example: 07:45 → 08:30 but distance = 1200+ km
    if(diff >= 0 && distance >= 1200 && diff < 12*60)
    {
        diff += 24*60;
    }

    int hr = diff / 60;
    int mn = diff % 60;

    return to_string(hr) + " hr " + to_string(mn) + " min";
}

vector<string> getSmartIntermediate(const string& src, const string& dest) {
    vector<string> s;

    string uSrc = src, uDest = dest;
    transform(uSrc.begin(), uSrc.end(), uSrc.begin(), ::toupper);
    transform(uDest.begin(), uDest.end(), uDest.begin(), ::toupper);

    if (uSrc.find("MADGAON") != string::npos && uDest.find("MUMBA") != string::npos) {
        return {"KARWAR", "UDUPI"};
    }
    if (uSrc.find("MADGAON") != string::npos && uDest.find("ERNAKULAM") != string::npos) {
        return {"KARWAR", "MANGALORE"};
    }
    if (uSrc.find("MUMBA") != string::npos && uDest.find("PUNE") != string::npos) {
        return {"DADAR", "THANE"};
    }
    if (uSrc.find("GOA") != string::npos && uDest.find("KERALA") != string::npos) {
        return {"KARWAR", "MANGALORE"};
    }

    // Default fallback
    return {};
}


void displayDirectTrainsBeautiful(const vector<Train>& trains,
                                  const string& origin,
                                  const string& dest)
{
    if (trains.empty()) {
        cout << "\n❌ No direct trains between " << origin << " → " << dest << "\n";
        return;
    }

    cout << "\n==============================\n";
    cout << "🚆 DIRECT TRAINS: " << origin << " → " << dest << "\n";
    cout << "==============================\n\n";

    for (const auto& t : trains) {
        cout << "🚆 " << t.trainNumber << " – " << t.trainName << "\n";
       // cout << "⏱ " << t.departure << " → " << t.arrival << "\n";
        string dep = (!t.departure.empty())
             ? t.departure
             : (t.departureTimes.empty() ? "00:00" : t.departureTimes[0]);

string arr = (!t.arrival.empty())
             ? t.arrival
             : (t.arrivalTimes.empty() ? "00:00" : t.arrivalTimes.back());

        cout << "🕒 Duration: " << (t.duration / 60) << " hr "
             << (t.duration % 60) << " min\n";
        cout << "📏 Distance: " << t.distance << " km\n";
        cout << "🏷 Zone: " << t.zone << " | Type: " << t.type << "\n";
        cout << "--------------------------------------\n";
    }
}
