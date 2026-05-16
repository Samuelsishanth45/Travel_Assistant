// station_mapper.cpp
// Enhanced station mapper: robust normalization, substring/token matching,
// and fuzzy (Levenshtein) fallback. Returns station codes when available.
// If no reliable mapping found, returns empty string (""), so caller will use Google Maps fallback.

#include "station_mapper.h"
#include <iostream>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <climits>
#include <vector>
#include <string>
#include <cctype>
#include <cmath>
#include <set>
#include <iomanip>
#include "train_matrix.h"


using namespace std;

// ---------------------- Helpers ------------------------

/*
string resolveCityToBestStation(const string& input) {
    string city = input;
    transform(city.begin(), city.end(), city.begin(), ::toupper);

    string bestStation = "";
    int bestScore = -1;

    for (const auto& p : stationUsageCount) {
        if (p.first.find(city) != string::npos) {
            if (p.second > bestScore) {
                bestScore = p.second;
                bestStation = p.first;
            }
        }
    }

    return bestStation; // empty if none
}
*/

static unordered_map<string, string> cityToHub = {
    {"MUMBAI", "BCT"},
    {"DELHI", "NDLS"},
    {"NEWDELHI", "NDLS"},
    {"CHENNAI", "MAS"},
    {"KOLKATA", "HWH"},
    {"HOWRAH", "HWH"},
    {"BENGALURU", "SBC"},
    {"BANGALORE", "SBC"},
    {"HYDERABAD", "HYB"},
    {"SECUNDERABAD", "SC"},
    {"PUNE", "PUNE"},
    {"GOA", "MAO"},
    {"MADGAON", "MAO"},
    {"VIJAYAWADA", "BZA"},
    {"VISAKHAPATNAM", "VSKP"},
    {"VIZAG", "VSKP"},
    {"KOCHI", "ERS"},
    {"ERNAKULAM", "ERS"},
    {"TRIVANDRUM", "TVC"},
    {"THIRUVANANTHAPURAM", "TVC"},
    {"JAIPUR", "JP"},
    {"AHMEDABAD", "ADI"},
    {"SURAT", "ST"},
    {"BHOPAL", "BPL"},
    {"INDORE", "INDB"},
    {"NAGPUR", "NGP"},
    {"PATNA", "PNBE"},
    {"LUCKNOW", "LKO"},
    {"KANPUR", "CNB"},
    {"VARANASI", "BSB"}
};


static string up(const string &s) {
    string out; out.reserve(s.size());
    for (unsigned char c : s) out.push_back((char)toupper(c));
    return out;
}

static string stripPunctAndCollapse(const string &s) {
    string tmp;
    tmp.reserve(s.size());
    for (unsigned char ch : s) {
        if (isalnum(ch) || isspace(ch)) tmp.push_back((char)ch);
        else tmp.push_back(' ');
    }
    // collapse multi-spaces
    string out;
    istringstream iss(tmp);
    string token;
    bool first = true;
    while (iss >> token) {
        if (!first) out.push_back(' ');
        first = false;
        out += token;
    }
    return out;
}

static vector<string> tokenize(const string &s) {
    vector<string> t;
    istringstream iss(s);
    string tok;
    while (iss >> tok) t.push_back(tok);
    return t;
}

static string trim(const string &s) {
    size_t a = s.find_first_not_of(" \t\n\r");
    if (a==string::npos) return "";
    size_t b = s.find_last_not_of(" \t\n\r");
    return s.substr(a, b-a+1);
}

// strip common suffixes like "Railway Station", "Junction", "Bus Stand", "Station", "Terminal"
static string stripCommonSuffixes(const string &raw) {
    string s = up(stripPunctAndCollapse(raw));
    vector<string> suffixes = {
        " RAILWAY STATION", " RAILWAY", " RAILWAY STN", " JUNCTION", " JN", " JN.", " STATION",
        " BUS STAND", " BUS STOP", " BUS STN", " CENTRAL", " TERMINUS", " TERMINAL", " BUS TERMINAL",
        " RLY STN", " RLY", " DEPOT"
    };
    for (const auto &suf : suffixes) {
        size_t pos = s.find(suf);
        if (pos != string::npos) {
            s = trim(s.substr(0, pos));
            break;
        }
    }
    return trim(s);
}
/*
// Levenshtein distance (classic DP) — small strings only so fine
static int levenshtein(const string &a, const string &b) {
    if (a == b) return 0;
    int na = (int)a.size(), nb = (int)b.size();
    if (na == 0) return nb;
    if (nb == 0) return na;
    vector<int> prev(nb+1), cur(nb+1);
    for (int j=0;j<=nb;++j) prev[j] = j;
    for (int i=1;i<=na;++i) {
        cur[0] = i;
        for (int j=1;j<=nb;++j) {
            int cost = (a[i-1]==b[j-1])?0:1;
            cur[j] = min({ prev[j] + 1, cur[j-1] + 1, prev[j-1] + cost });
        }
        prev.swap(cur);
    }
    return prev[nb];
}


// Token overlap score: how many tokens in common / average tokens
static double tokenOverlapScore(const vector<string>& A, const vector<string>& B) {
    if (A.empty() || B.empty()) return 0.0;
    set<string> sa(A.begin(), A.end()), sb(B.begin(), B.end());
    int common = 0;
    for (const auto &x : sa) if (sb.count(x)) ++common;
    double denom = (sa.size() + sb.size())/2.0;
    if (denom <= 0.0) return 0.0;
    return common / denom;
}
*/
// ---------------------- Station database ------------------------
// Minimal built-in mapping — keep this but you should expand by loading a file later.
// Keys are uppercase cleaned names (no suffixes). Values are station codes.
static unordered_map<string,string> &getStaticStationMap() {
    static unordered_map<string,string> mapdb = {
        // keep many entries here (you already had a large list). Add more as needed.
        /*
        {"VIJAYAWADA","BZA"},
        {"VIJAYAWADA JUNCTION","BZA"},
        {"MADGAON","MAO"},
        {"MADGAON JUNCTION","MAO"},
        {"GOA","MAO"},
        {"MUMBAI","BCT"},
        {"MUMBAI","ARA"},
        {"ARA","MUMBAI CENTRAL"},
        {"MUMBAI CENTRAL","BCT"},
        {"MUMBAI CST","BCT"},
        {"CHHATRAPATI SHIVAJI MAHARAJ TERMINUS","CSTM"},
        {"CHHATRAPATI SHIVAJI MAHARAJ TERMINUS","CSMT"},
        {"CHHATRAPATI SHIVAJI MAHARAJ TERMINUS CSMT","CSMT"},
        {"PUNE","PUNE"},
        {"PUNE JUNCTION","PUNE"},
        {"NEW DELHI","NDLS"},
        {"DELHI","NDLS"},
        {"CHENNAI","MAS"},
        {"ERNAKULAM","ERS"},
        {"KOCHI","ERS"},
        {"KANNUR","CAN"},
        {"ALLEPPEY","ALL"},
        {"ALAPPUZHA","ALL"},
        {"BANGALORE","SBC"},
        {"BENGALURU","SBC"},
        {"YESVANTPUR","YPR"},
        {"HYDERABAD","HYB"},
        {"SECUNDERABAD","SC"},
        {"VISAKHAPATNAM","VSKP"},
        {"VIZAG","VSKP"},
        {"GODOWN",""}, // placeholder examples
        // ... extend this database with the rest of your 700+ stations ...
        */
    };
    return mapdb;
}

/*

// Utility to find best matching station code for a cleaned name
static string findBestInStaticDB(const string &cleanName) {
    if (cleanName.empty()) return "";

    auto &stationMap = getStaticStationMap();

    // 1) Exact match
    auto it = stationMap.find(cleanName);
    if (it != stationMap.end()) {
        if (!it->second.empty()) return it->second;
    }

    // 2) Try case where original key contains cleanName (partial)
    for (const auto &p : stationMap) {
        if (p.first.find(cleanName) != string::npos) {
            if (!p.second.empty()) return p.second;
        }
    }

    // 3) Token overlap scoring
    vector<string> tokensQuery = tokenize(cleanName);
    double bestScore = 0.0;
    string bestCode = "";
    string bestKey = "";
    for (const auto &p : stationMap) {
        vector<string> tokensKey = tokenize(p.first);
        double score = tokenOverlapScore(tokensQuery, tokensKey);
        if (score > bestScore) {
            bestScore = score;
            bestCode = p.second;
            bestKey = p.first;
        }
    }
    if (bestScore >= 0.5 && !bestCode.empty()) { // reasonably strong overlap
        return bestCode;
    }

    // 4) Fuzzy (levenshtein) fallback across keys - compute normalized distance
    // We'll search a small candidate subset to be faster: only keys with first token equal to first token of query or with some overlap
    vector<pair<int,string>> levCandidates;
    for (const auto &p : stationMap) {
        int dist = levenshtein(cleanName, p.first);
        levCandidates.emplace_back(dist, p.second);
    }
    sort(levCandidates.begin(), levCandidates.end(), [](const pair<int,string>&a,const pair<int,string>&b){
        return a.first < b.first;
    });
    if (!levCandidates.empty()) {
        int bestDist = levCandidates.front().first;
        // Accept if distance is small relative to length
        int thresh = max(2, (int)cleanName.size()/6); // small tolerance
        if (bestDist <= thresh && !levCandidates.front().second.empty()) {
            return levCandidates.front().second;
        }
    }

    return "";
}
*/

string extractProperStationName(const string& googleName) {
    string name = googleName;
    // Remove common words that are just descriptors (but keep main name)
    vector<string> suffixes = {
        " Railway Station", " railway station",
        " Junction", " junction", " Jn", " jn", " JN.", " JN",
        " Station", " station",
        " Terminus", " terminus",
        " Central", " central",
        " Railway", " railway"
    };

    for (const auto &suff : suffixes) {
        size_t pos = name.find(suff);
        if (pos != string::npos) {
            name = name.substr(0, pos);
            break;
        }
    }

    name = trim(name);
    if (name.empty()) return googleName;
    return name;
}

// New resolveStationName: do NOT convert to station CODE.
// Return a clean human-readable station/place name for route planning.
// Use this for Google Maps-based options (2,7 and option 3).
string resolveStationName(const string& input) {
    // If it's already short, just trim and return
    string name = trim(input);
    if (name.empty()) return input;

    // If google already returned something like "Madgaon Railway Station", clean it:
    string proper = extractProperStationName(name);

    return proper;
}
string findStationCode(const string& input) {
    if (input.empty()) return "";

    string key = input;
    transform(key.begin(), key.end(), key.begin(), ::toupper);

    vector<string> junk = {
        " RAILWAY STATION", " JUNCTION", " JN", " STATION"
    };
    for (auto &j : junk) {
        size_t pos;
        while ((pos = key.find(j)) != string::npos)
            key.erase(pos, j.length());
    }

    while (!key.empty() && key.front() == ' ') key.erase(key.begin());
    while (!key.empty() && key.back() == ' ') key.pop_back();

    auto it = stationNameToCode.find(key);
    if (it != stationNameToCode.end())
        return it->second;

    return "";
}


/*
string findStationCode(const string& input) {
    if (input.empty()) return "";

    string clean = input;
    transform(clean.begin(), clean.end(), clean.begin(), ::toupper);

    // 1️⃣ Exact station or code match
    auto it = stationNameToCode.find(clean);
    if (it != stationNameToCode.end())
        return it->second;

    // 2️⃣ City → BEST station (MOST USED)
    string cityBest = resolveCityToBestStation(clean);
    if (!cityBest.empty())
        return cityBest;

    return "";
}
*/


/*
string resolveCityToBestStation(const string& input) {
    string city = input;
    transform(city.begin(), city.end(), city.begin(), ::toupper);

    if (cityToStations.find(city) == cityToStations.end())
        return "";

    string bestStation = "";
    int bestCount = -1;

    for (const auto& code : cityToStations[city]) {
        int count = stationUsageCount[code];
        if (count > bestCount) {
            bestCount = count;
            bestStation = code;
        }
    }

    return bestStation;
}
*/

string resolveCityToBestStation(const string& input) {
    string city = input;
    transform(city.begin(), city.end(), city.begin(), ::toupper);

    string bestCode = "";
    int bestCount = 0;

    for (const auto& p : stationNameToCode) {
        if (p.first.find(city) == 0) { // prefix match
            int count = stationUsageCount[p.second];
            if (count > bestCount) {
                bestCount = count;
                bestCode = p.second;
            }
        }
    }
    return bestCode;
}


// Suggest similar station names from DB (useful to show to user)
vector<string> findSimilarStations(const string& input) {
    vector<string> matches;
    auto stationMap = loadStationDatabase(); // keys like "MUMBAI", "VIJAYAWADA" etc.

    string up = input;
    transform(up.begin(), up.end(), up.begin(), ::toupper);
    up = trim(up);

    for (const auto &pair : stationMap) {
        string key = pair.first; // already upper-case common names
        if (key.find(up) != string::npos || up.find(key) != string::npos) {
            matches.push_back(pair.first); // return readable key (e.g., "MADGAON")
        }
    }

    sort(matches.begin(), matches.end());
    matches.erase(unique(matches.begin(), matches.end()), matches.end());
    // Limit to top 10 suggestions
    if (matches.size() > 10) matches.resize(10);
    return matches;
}

// Map station code to city name (used for display)
string getStationCity(const string &stationCode) {
    static unordered_map<string,string> codeToCity = {
        {"BZA","Vijayawada"}, {"BCT","Mumbai"}, {"NDLS","Delhi"}, {"MAS","Chennai"},
        {"HWH","Kolkata"}, {"SBC","Bengaluru"}, {"HYB","Hyderabad"}, {"PUNE","Pune"},
        {"MAO","Goa"}, {"ERS","Kochi"}, {"VSKP","Visakhapatnam"}, {"NLR","Nellore"},
        {"PNBE","Patna"}, {"BSB","Varanasi"}, {"CNB","Kanpur"}
    };
    auto it = codeToCity.find(stationCode);
    if (it != codeToCity.end()) return it->second;
    return stationCode;
}
// FINAL IMPLEMENTATION OF loadStationDatabase()
// Exposes the internal static station map as a normal unordered_map
std::unordered_map<std::string, std::string> loadStationDatabase() {
    auto &m = getStaticStationMap();     // your internal DB
    // Convert to normal unordered_map<std::string,std::string>
    std::unordered_map<std::string, std::string> out;
    for (const auto &p : m) {
        out[p.first] = p.second;
    }
    return out;
}
