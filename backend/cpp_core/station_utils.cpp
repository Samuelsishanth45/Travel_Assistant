#include "station_utils.h"
// station_utils.cpp — adjusted formatting & matching
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>
#include <map>
#include <iostream>
using namespace std;

static inline string toUpperCopy(const string &s) {
    string t = s;
    transform(t.begin(), t.end(), t.begin(), ::toupper);
    return t;
}

string normalize(string s) {
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    s.erase(remove_if(s.begin(), s.end(), ::isspace), s.end());
    return s;
}


std::string formatStationName(const std::string& stationName) {
    if (stationName.empty()) return stationName;
    // Trim leading/trailing spaces but keep case intact for display
    size_t start = stationName.find_first_not_of(" ");
    size_t end = stationName.find_last_not_of(" ");
    if (start == string::npos) return "";
    return stationName.substr(start, end - start + 1);
}

bool isStationMatch(const std::string& input, const std::string& station) {
    if (input.empty() || station.empty()) return false;

    string formattedInput = formatStationName(input);
    string formattedStation = formatStationName(station);

    string inU = toUpperCopy(formattedInput);
    string stU = toUpperCopy(formattedStation);

    // Remove common suffixes for matching purposes
    vector<string> suffixes = {
        " JUNCTION", " JN", " JN.", " RAILWAY STATION", " STATION", " CENTRAL", " TERMINUS", " CANTT", " CANT"
    };
    for (const auto &suf : suffixes) {
        size_t pos;
        pos = inU.find(suf);
        if (pos != string::npos) inU = inU.substr(0, pos);
        pos = stU.find(suf);
        if (pos != string::npos) stU = stU.substr(0, pos);
    }

    // Trim remaining spaces
    auto trimU = [](string &x) {
        size_t a = x.find_first_not_of(" ");
        if (a == string::npos) { x = ""; return; }
        size_t b = x.find_last_not_of(" ");
        x = x.substr(a, b - a + 1);
    };
    trimU(inU); trimU(stU);
    if (inU.empty() || stU.empty()) return false;

    if (inU == stU) return true;
    if (stU.find(inU) != string::npos) return true;
    if (inU.find(stU) != string::npos) return true;

    // First word match
    string inFirst = inU.substr(0, inU.find(' '));
    string stFirst = stU.substr(0, stU.find(' '));
    if (!inFirst.empty() && inFirst == stFirst) return true;

    return false;
}

string normalizeStationName(string s) {
    transform(s.begin(), s.end(), s.begin(), ::toupper);

    vector<string> junk = {
        " JUNCTION", " JN.", " JN", " RLY",
        " RAILWAY STATION", " STATION", ".", " ROAD"
    };

    for (auto &j : junk) {
        size_t pos;
        while ((pos = s.find(j)) != string::npos)
            s.erase(pos, j.length());
    }

    while (!s.empty() && s.front() == ' ') s.erase(s.begin());
    while (!s.empty() && s.back() == ' ') s.pop_back();

    return s;
}


std::vector<std::string> getAllStations(const std::vector<Train>& trains) {
    std::vector<std::string> list;
    for (const auto& t : trains) {
        list.push_back(formatStationName(t.source));
        list.push_back(formatStationName(t.destination));
    }
    std::sort(list.begin(), list.end());
    list.erase(std::unique(list.begin(), list.end()), list.end());
    return list;
}


std::map<std::string, GraphNode*> buildGraph(const std::vector<Train>& trains) {
    std::map<std::string, GraphNode*> graph;

    for (const auto& t : trains) {
        if (!graph.count(t.source))
            graph[t.source] = new GraphNode(t.source);
        if (!graph.count(t.destination))
            graph[t.destination] = new GraphNode(t.destination);

        unsigned long cost = (t.duration > 0) ? t.duration * 60 : 3600;

        graph[t.source]->neighbors.push_back({ t.destination, cost });
    }

    return graph;
}

void cleanupGraph(std::map<std::string, GraphNode*>& graph) {
    for (auto& p : graph) delete p.second;
    graph.clear();
}


string cleanTrainName(string name) {
    if (name.empty()) return "Passenger / Express";

    transform(name.begin(), name.end(), name.begin(), ::toupper);

    vector<string> junk = {
        " EXP", " EXPRESS", " SF", " SUPERFAST",
        " MAIL", " PASS", " PASSENGER", " SPL", " SPECIAL"
    };

    for (auto &j : junk) {
        size_t pos;
        while ((pos = name.find(j)) != string::npos)
            name.erase(pos, j.length());
    }

    // collapse spaces
    string out;
    bool space = false;
    for (char c : name) {
        if (c == ' ') {
            if (!space) out += c;
            space = true;
        } else {
            out += c;
            space = false;
        }
    }

    return out;
}
