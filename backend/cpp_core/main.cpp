#include <iostream>
#include <limits>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <map>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <string>

//#include "csv_parser.h"
#include "train_matrix.h"
#include "journey_planner.h"
#include "csv_parser.h"
#include "graph.h"
#include "tourist.h"
#include "maps_api.h"
#include "direct_trains.h"
#include "intelligent_routes.h"
#include "station_mapper.h"
#include <typeinfo>
#include "direct_trains.h"
#include "json_parser.h"
#include "direct_trains.h"
#include "station_mapper.h"
#include "station_utils.h"
#include "station_mapper.h"
#include "indirectroutes.h"
#include "floyd_warshall.h"

using namespace std;
using namespace journey_planner;


// Function declarations
static std::vector<std::string> emptyStations;

int getValidIntInput(const string& prompt, int minVal, int maxVal);
//string getStationInput(const string& prompt, const vector<string>& stations);
void displayDirectTrains(const vector<pair<string, pair<int, string>>>& directTrains, 
                        const vector<Train>& allTrains,
                        const string& source, const string& destination);
void displayWelcome();
void displayMenu();
//vector<JourneyStep> journey = getRealTimeJourneyPlan(source, destination, allTrains);

std::string source, destination;
//std::vector<Train> allTrains;
//vector<JourneyStep> getRealTimeJourneyPlan(const std::string& source, const std::string& destination, const std::vector<Train>& trains);

vector<string> getAllStations(const vector<Train>& trains); // KEEP THIS DECLARATION
void displayStationsAtLocation(const string& userLocation);
void openUrlInBrowser(const string& url);
//vector<JourneyStep> getRealTimeJourneyPlan(const string& source, const string& destination);
vector<JourneyStep> createBusAlternativePlan(const string& source, const string& destination);
void displayRealTimeJourney(const vector<JourneyStep>& steps, const string& source, const string& destination);
void planRealTimeJourney();

//void generateSmartTravelPlan(string city, int days);
void planSmartTour(const string& city, int days);

#include <locale.h>
#include <vector>
#include <string>

int getValidIntInput(const string& prompt, int minVal, int maxVal) {
    int value;
    string input;
    
    while (true) {
        cout << prompt;
        getline(cin, input);
        
        stringstream ss(input);
        if (ss >> value && ss.eof() && value >= minVal && value <= maxVal) {
            return value;
        }
        
        cout << "Invalid input! Please enter a number between " << minVal << " and " << maxVal << ".\n";
        cin.clear();
    }
}
// In main.cpp - SIMPLIFY the getStationInput function
// AI-powered station input using Google Maps

string getStationInput(const string& prompt, const vector<string>& stations, bool useLocalDB) {
    string input;
    while (true) {
        cout << prompt;
        getline(cin, input);

        if (input.empty()) {
            cout << "❌ Please enter a location." << endl;
            continue;
        }

        cout << "🔍 Finding nearest railway/bus place..." << endl;
        // Ask Google Maps for nearest station/place
        string googleNearest = findNearestStation(input); // returns readable name from maps_api

        if (googleNearest.empty()) {
            cout << "❌ Google Maps couldn't find a place for '" << input << "'. Try a larger city or landmark." << endl;
            continue;
        }

        // Clean the Google result (remove 'Railway Station' suffix only visually)
        string cleaned = resolveStationName(googleNearest);

        cout << "📍 Google Maps suggests: " << googleNearest << endl;
        cout << "   Using as: " << cleaned << endl;

       // if (!useLocalDB) {
            // For options 2,7,3 -> use Google Maps result directly (no code mapping)
            //return cleaned;}

        return cleaned;


        // useLocalDB == true -> try to match to local station names in 'stations' vector
        // Prefer exact/local matches
        for (const auto &st : stations) {
            if (isStationMatch(cleaned, st) || isStationMatch(st, cleaned)) {
                cout << "✅ Matched to local DB station: " << st << endl;
                return st;
            }
        }

        // if not matched, suggest similar stations from DB
        vector<string> similar;
        for (const auto &st : stations) {
            string stClean = formatStationName(st);
            string stU = stClean; transform(stU.begin(), stU.end(), stU.begin(), ::toupper);
            string cleanedU = cleaned; transform(cleanedU.begin(), cleanedU.end(), cleanedU.begin(), ::toupper);
            if (stU.find(cleanedU) != string::npos || cleanedU.find(stU) != string::npos) {
                similar.push_back(st);
            }
        }

        if (!similar.empty()) {
            cout << "🔍 Similar stations found in local DB. Using closest match: " << similar[0] << endl;
            return similar[0];
        }
/*
        // If we reach here, no local match — ask user whether to use Google result anyway
        cout << "⚠️ Station not in local DB. Use Google Maps place for routing anyway? (y/n): ";
        char ch;
        cin >> ch;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        if (ch == 'y' || ch == 'Y') return cleaned;

        cout << "Okay — try typing a different city or a known station name." << endl;
*/
        cout << "⚠️ Station not found in Indian Railways DB.\n";
cout << "💡 Tip: Try a major station or nearby city.\n";
continue;

    }
}


// Function to open URL in default browser (cross-platform with better Linux support)
void openUrlInBrowser(const string& url) {
    cout << "🌐 Attempting to open browser..." << endl;
    
    string command;
    int result = -1;
    
    #ifdef _WIN32
        command = "start \"" + url + "\"";
        result = system(command.c_str());
        
    #elif __APPLE__
        command = "open \"" + url + "\"";
        result = system(command.c_str());
        
    #else
        // Linux: Try multiple methods
        const char* browsers[] = {
            "wslview", "xdg-open", "gnome-open", "kde-open", 
            "x-www-browser", "sensible-browser", "firefox", "google-chrome", "chromium-browser"
        };
        
        for (const char* browser : browsers) {
            cout << "🔧 Trying " << browser << "..." << endl;
            command = string("which ") + browser + " > /dev/null 2>&1";
            if (system(command.c_str()) == 0) {
                command = string(browser) + " \"" + url + "\" &";
                result = system(command.c_str());
                if (result == 0) {
                    cout << "✅ Successfully opened with " << browser << endl;
                    return;
                }
            }
        }
        
        // If no browser found, try direct execution without checking
        cout << "🔧 Trying direct browser execution..." << endl;
        const char* directBrowsers[] = {"firefox", "google-chrome", "chromium-browser"};
        for (const char* browser : directBrowsers) {
            command = string(browser) + " \"" + url + "\" &";
            result = system(command.c_str());
            if (result == 0 || result == 256) { // 256 often means success in background
                cout << "✅ Opened with " << browser << endl;
                return;
            }
        }
    #endif
    
    // If we reach here, automatic opening failed
    cout << "❌ Could not automatically open browser." << endl;
    cout << "📋 Please manually visit this URL:" << endl;
    cout << "🔗 " << url << endl;
    
    // Alternative: Offer to copy to clipboard if xclip is available
    #ifndef _WIN32
    if (system("which xclip > /dev/null 2>&1") == 0) {
        cout << "\nWould you like to copy the URL to clipboard? (y/n): ";
        char choice;
        cin >> choice;
        if (choice == 'y' || choice == 'Y') {
            command = "echo \"" + url + "\" | xclip -selection clipboard";
            int ret = system(command.c_str());
if (ret != 0) {
    std::cerr << "⚠️  Could not open browser (code " << ret << ")" << std::endl;
}

          //  (void)system(command.c_str());

           // system(command.c_str());
            cout << "✅ URL copied to clipboard!" << endl;
        }
    }
    #endif
}





// Replace existing displayStationsAtLocation(...) with this version
// Replace your existing displayStationsAtLocation(...) with this version
void displayStationsAtLocation(const string& userLocation, bool isAPI) {
    cout << "\n📍 Searching for railway stations near: " << userLocation << endl;
    cout << "Please wait while we fetch real-time data from Google Maps..." << endl;

    // Primary attempt: Google Maps nearby search (existing signature)
    vector<Station> stations = findStationsByLocation(userLocation);

    // Helper lambda to ensure a station has a usable display name/address
    auto ensureStationName = [&](Station &s, const string &fallbackText){
        // Trim whitespace quickly
        auto trim = [](string str){
            size_t a = str.find_first_not_of(" \t\n\r");
            if (a==string::npos) return string("");
            size_t b = str.find_last_not_of(" \t\n\r");
            return str.substr(a, b-a+1);
        };

        if (trim(s.name).empty()) {
            if (!trim(s.address).empty()) {
                s.name = s.address;
            } else if (!fallbackText.empty()) {
                s.name = fallbackText;
            } else {
                s.name = "Unknown Railway Station";
            }
        }
        if (trim(s.address).empty() && !s.name.empty() && s.name.find("Railway")==string::npos) {
            s.address = s.name + ", India";
        }
    };

    // If nearby search returned nothing, try Google single suggestion
    if (stations.empty()) {
        cout << "⚠️ Nearby search returned no results. Requesting Google suggestion...\n";
        string suggestion = findNearestStation(userLocation);
        if (!suggestion.empty() && suggestion != userLocation) {
            Station s;
            s.name = suggestion;
            s.address = suggestion + ", India";
            s.distance = 0.0;
            s.latitude = 0.0;
            s.longitude = 0.0;
            stations.push_back(s);
            cout << "🔎 Google suggests: " << suggestion << endl;
        }
    }

    // If still empty or the returned station(s) are missing names, try offline similarity DB
    if (stations.empty() || (stations.size() > 0 && stations[0].name.empty())) {
        cout << "⚠️ Suggestion not found or incomplete. Falling back to offline station DB...\n";
        vector<string> similar = findSimilarStations(userLocation); // station_mapper.cpp

        if (similar.empty()) {
            // conservative defaults (small towns will be handled by later UI)
            similar = {"Vijayawada", "Guntur", "Visakhapatnam", "Hyderabad", "Madgaon"};
        }

        // Build Station objects for the top 5 similar names (only if we currently have none)
        if (stations.empty()) {
            for (size_t i = 0; i < similar.size() && stations.size() < 5; ++i) {
                Station s;
                s.name = similar[i] + " Railway Station";
                s.address = similar[i] + ", India";
                s.distance = 0.0;
                s.latitude = 0.0;
                s.longitude = 0.0;
                stations.push_back(s);
            }
        } else {
            // Ensure first station has a readable name
            ensureStationName(stations[0], similar.size() ? (similar[0] + " Railway Station") : "Nearest Railway Station");
        }
    }

    // Final guard: ensure we have at least one usable station
    if (stations.empty()) {
        cout << "\n❌ Could not find any stations near: " << userLocation << endl;
        cout << "💡 Tip: Try entering a nearby town/city or add the state (e.g., 'Ponda, Goa').\n";
        return;
    }

    // Limit to 5 candidates
    if (stations.size() > 5) stations.resize(5);

    // Ensure every station has a proper display name/address
    for (size_t i = 0; i < stations.size(); ++i) {
        // If station has no name, try sensible fallbacks
        string fallback = "Station " + to_string(i+1);
        ensureStationName(stations[i], fallback);
    }

    // Display the list and prepare URLs
    cout << "\n" << string(60, '=') << endl;
    cout << "     🚉 RAILWAY STATIONS NEAR: " << userLocation << endl;
    cout << string(60, '=') << endl;

    vector<string> directionsUrls;
    for (size_t i = 0; i < stations.size(); ++i) {
        const Station &st = stations[i];

        cout << "\n" << (i + 1) << ". 🚉 " << st.name << endl;
        if (!st.address.empty()) cout << "   📍 " << st.address << endl;
        if (st.distance > 0.0) {
            cout << "   📏 " << fixed << setprecision(1) << st.distance << " km away" << endl;
        } else {
            cout << "   📏 Distance: N/A" << endl;
        }
        if (st.latitude != 0.0 || st.longitude != 0.0) {
            cout << "   🌐 Coords: " << st.latitude << ", " << st.longitude << endl;
        }

        // Build a safe destination param:
        string destParam = st.name;
        // If the name looks minimal, prefer the address; if both minimal, append "Railway Station"
        if (destParam.empty() || destParam == "Unknown Railway Station" || destParam == "Station " + to_string(i+1)) {
            if (!st.address.empty()) destParam = st.address;
            else destParam = userLocation + " Railway Station";
        }
        // Ensure "Railway Station" present so Maps picks the right POI
        if (destParam.find("Railway") == string::npos && destParam.find("railway") == string::npos) {
            destParam += " Railway Station";
        }

        string url = getDirectionsUrl(userLocation, destParam);
        directionsUrls.push_back(url);

        cout << "   🔗 Directions: " << url << endl;
    }

    cout << "\n" << string(60, '=') << endl;
    cout << "✅ Found " << stations.size() << " station(s) near your location." << endl;



cout << "STATIONS_START\n";

for (size_t i = 0; i < stations.size(); ++i) {
    const Station &st = stations[i];

    cout << "STATION_START\n";

    cout << "name:" << st.name << "\n";
    cout << "address:" << st.address << "\n";

    if (st.distance > 0.0)
        cout << "distance:" << fixed << setprecision(1) << st.distance << "\n";
    else
        cout << "distance:-1\n";

    if (st.latitude != 0.0 && st.longitude != 0.0)
        cout << "coords:" << st.latitude << "," << st.longitude << "\n";
    else
        cout << "coords:-\n";

    string destParam = st.name;
    if (destParam.find("Railway") == string::npos)
        destParam += " Railway Station";

    string url = getDirectionsUrl(userLocation, destParam);

    cout << "url:" << url << "\n";

    cout << "STATION_END\n";
}

cout << "STATIONS_END\n";




    /*
    // Ask user which station they want directions to
    cout << "\n🎯 Which station would you like directions to? Enter number (1-" << stations.size() << ") or 0 to cancel: ";
    string line;
    getline(cin, line);
    if (line.empty()) {
        // If user pressed enter immediately, ask once more
        cout << "Enter choice (1-" << stations.size() << ", 0 to cancel): ";
        getline(cin, line);
    }

    int choice = -1;
    try { choice = stoi(line); } catch(...) { choice = -1; }

    if (choice >= 1 && choice <= (int)stations.size()) {
        int idx = choice - 1;
        cout << "🌐 Opening Google Maps directions to: " << stations[idx].name << " ..." << endl;
        openUrlInBrowser(directionsUrls[idx]);
    } else {
        cout << "ℹ️ No directions opened. You can copy any of the URLs above manually.\n";
    }
    */
}






void displayDirectTrains(const vector<pair<string, pair<int, string>>>& directTrains, 
                        const vector<Train>& allTrains,
                        const string& source, const string& destination) {
    if (directTrains.empty()) {
        cout << "\n" << string(50, '=') << endl;
        cout << "   🚫 No Direct Trains Found" << endl;
        cout << string(50, '=') << endl;
        
        cout << "\n💡 Smart Suggestions:" << endl;
        cout << "   1. Try Option 2 for alternative routes" << endl;
        cout << "   2. Check station names" << endl;
        cout << "   3. Use major city names" << endl;
        
        return;
    }
    
    cout << "\n" << string(70, '=') << endl;
    cout << "   🚆 REAL-TIME TRAIN INFORMATION" << endl;
    cout << "   From: " << source << " → To: " << destination << endl;
    cout << string(70, '=') << endl;
    
    cout << "┌──────────────────────────┬────────────┬────────────┬──────────────┬──────────┐" << endl;
    cout << "│ Train Name               │ Depart     │ Arrival    │ Duration     │ Distance │" << endl;
    cout << "├──────────────────────────┼────────────┼────────────┼──────────────┼──────────┤" << endl;
    
    for (const auto& train : directTrains) {
        string info = train.second.second;
        vector<string> details;
        stringstream ss(info);
        string token;
        
        while (getline(ss, token, '|')) {
            details.push_back(token);
        }
        
        if (details.size() >= 4) {
            string trainName = train.first;
            if (trainName.length() > 24) {
                trainName = trainName.substr(0, 24) + "..";
            }
            
            printf("│ %-24s │ %-10s │ %-10s │ %-12s │ %-8s │\n",
                   trainName.c_str(),
                   details[0].c_str(), details[1].c_str(),
                   details[2].c_str(), (to_string(train.second.first) + " km").c_str());
        }
    }
    
    cout << "└──────────────────────────┴────────────┴────────────┴──────────────┴──────────┘" << endl;
    
    cout << "\n💡 Real-time information provided by Google Maps" << endl;
    cout << "📋 Next Steps:" << endl;
    cout << "   1. Check current schedules on IRCTC" << endl;
    cout << "   2. Use Option 6 for station directions" << endl;
    cout << "   3. Verify timings before travel" << endl;

    // Show tourist attractions
    cout << "\n🏛️  Tourist attractions in " << destination << ":" << endl;
    displayTouristAttractions(destination);
}

void displayWelcome() {
    cout << "\n================================================" << endl;
    cout << "        INDIAN RAILWAYS TOURIST GUIDE" << endl;
    cout << "     Discover India's Rich Heritage & Culture" << endl;
    cout << "     With Real-time Google Maps Integration!" << endl;
    cout << "================================================" << endl;
}

//int main() {
int main(int argc, char* argv[]) {

    ios::sync_with_stdio(false);
    cin.tie(NULL);

    // 🚀 LOAD DATA
    loadTrainsFromCSV("trains.csv");
    //loadTrainsFromCSV("../trains.csv");

    bool isAPI = (argc >= 4);

    // 🎯 ARGUMENT VALIDATION
    if (argc < 4) {
        cout << "ERROR: Invalid arguments\n";
        return 1;
    }

    int choice = stoi(argv[1]);
    string src = argv[2];
    string dest = argv[3];

    transform(src.begin(), src.end(), src.begin(), ::toupper);
    transform(dest.begin(), dest.end(), dest.begin(), ::toupper);


 map<string, GraphNode*> graph;

  //int choice;
    string source, destination, city, userLocation;



    // 🎨 CLEAN HEADER (SERVER-FRIENDLY)
   // cout << "========================================\n";
    cout << "INDIAN RAILWAYS JOURNEY PLANNER\n";
    //cout << "========================================\n";

    cout << "SOURCE: " << src << "\n";

    cout << "DESTINATION: " << dest << "\n";
    //cout << "----------------------------------------\n";

    // =========================================================
    // 🔥 CASE 1 — DIRECT TRAINS
    // =========================================================
    if (choice == 1) {

     cout << "\n🎯 DIRECT TRAIN FINDER" << endl;

//string source, destination, srcCode, dstCode;
string source = isAPI ? src : "";

if (!isAPI) {
    cout << "Enter source station: ";
    cin >> source;
}

string srcCode = findStationCode(source);

if (srcCode.empty()) {
    cout << "❌ Station not found.\n";
    return 0;
}

cout << "✅ Station identified: "
     << getStationCity(srcCode) << " (" << srcCode << ")\n";

// DESTINATION
string destination = isAPI ? dest : "";

if (!isAPI) {
    cout << "Enter destination station: ";
    cin >> destination;
}

string dstCode = findStationCode(destination);

if (dstCode.empty()) {
    cout << "❌ Station not found.\n";
    return 0;
}

cout << "✅ Station identified: "
     << getStationCity(dstCode) << " (" << dstCode << ")\n";

// IMPORTANT: overwrite source & destination with CODES
source = srcCode;
destination = dstCode;
            auto directTrains = findDirectTrains(source, destination);


            if (directTrains.empty()) {
                cout << "\n🚫 No direct trains found.\n";
            } else {
                cout << "\n" << string(60, '=') << endl;
                cout << "🎯 DIRECT TRAINS FOUND! (" << directTrains.size() << " trains)\n";
                cout << "📍 From: " << source << " → To: " << destination << endl;
                cout << string(60, '=') << endl;

                sort(directTrains.begin(), directTrains.end(),
     [](const Train& a, const Train& b) {
         return a.distance < b.distance;   // or duration logic
     });


                for (size_t i = 0; i < directTrains.size() && i < 10; i++) {
                    const auto &train = directTrains[i];
string dep = (!train.departure.empty()) 
                ? train.departure 
                : (train.departureTimes.empty() ? "00:00" : train.departureTimes[0].substr(0,5));

string arr = (!train.arrival.empty()) 
                ? train.arrival 
                : (train.arrivalTimes.empty() ? "00:00" : train.arrivalTimes.back().substr(0,5));


//string dur = computeDuration(dep, arr);
string dur = computeDuration(dep, arr, (int)train.distance);
vector<string> mid = getSmartIntermediate(train.source, train.destination);



cout << "TRAIN_START\n";

cout << "name:" << cleanTrainName(train.trainName) << "\n";
cout << "number:" << train.trainNumber << "\n";
cout << "from:" << train.source << "\n";
cout << "to:" << train.destination << "\n";
cout << "departure:" << dep << "\n";
cout << "arrival:" << arr << "\n";
cout << "duration:" << dur << "\n";
cout << "distance:" << train.distance << "\n";
cout << "zone:" << train.zone << "\n";
cout << "type:" << train.type << "\n";

cout << "TRAIN_END\n";



                }
            }
// ------------------------------------------------------------
// ASK FOR INDIRECT ROUTES (OPTIONAL, REUSE SAME SRC & DST)
// ------------------------------------------------------------
if (directTrains.size() <= 8) {
   
if (!isAPI) {
    char ch;
    cout << "\nWould you like to explore indirect routes? (Y/N): ";
    cin >> ch;

    if (ch == 'Y' || ch == 'y') {
        handleIndirectRoutes(source, destination);
    }
}


}
        }

    // =========================================================
    // 🔥 CASE 2 — ALL ROUTES (INTELLIGENT ROUTES)
    // =========================================================
    else if (choice == 2) {
 cout << "\n🚂 INTELLIGENT JOURNEY PLANNER\n\n";

//string source = isAPI ? src : "";
//string destination = isAPI ? dest : "";
string source = isAPI ? src : "";
string destination = isAPI ? dest : "";

if (!isAPI) {
    cout << "Enter source station: ";
    cin >> source;

    cout << "Enter destination station: ";
    cin >> destination;
}

// ✅ NOW modify AFTER assignment
if (isAPI) {
    source += ", India";
    destination += ", India";
}


// 🚫 NO STATION FINDING FOR OPTION 2
string start = source;
string end = destination;

// Call our new logic
auto journeys = findIntelligentRoutes(start, end);
            if (journeys.empty()) {
                cout << "\n❌ No routes found.\n";
            } else {
                cout << "\n✅ Found " << journeys.size() << " route(s)\n";
               // for (size_t i = 0; i < journeys.size(); ++i)
                for (size_t i = 0; i < journeys.size() && i < 3; ++i) {
                    cout << "\n🏆 ROUTE " << (i+1) << ":\n";
                    displayJourneyPlan(journeys[i], i+1);



                }
            }
          
        }

    // =========================================================
    // 🔥 CASE 3 — REAL-TIME JOURNEY
    // =========================================================
  else if (choice == 3) {

    string userLocation = isAPI ? (src + ", India") : "";

    if (!isAPI) {
        cout << "Enter your current location: ";
        cin >> userLocation;
    }

    cout << "\n🔎 Searching nearby stations for: " << userLocation << "...\n";

    displayStationsAtLocation(userLocation, isAPI);

    cout << "\n✅ Done.\n";   // 🔥 FORCE END
}

    // =========================================================
    // 🔥 CASE 4 — BUS ALTERNATIVE
    // =========================================================
   else if (choice == 4) {

    string city = isAPI ? src : "";
    string daysStr = isAPI ? dest : "";

    int days;

    if (!isAPI) {
        cout << "\n🏙 Enter city: ";
        cin >> city;

        cout << "📅 Enter number of days: ";
        cin >> days;
    } else {
        if (!daysStr.empty()) days = stoi(daysStr);
    }

    planSmartTour(formatStationName(city), days);
  //  void planSmartTour(const string& city, int days)
}

    // =========================================================
    // 🔥 CASE 5 — TOURIST PLACES
    // =========================================================
    else if (choice == 5) {

       string city = isAPI ? src : "";

if (!isAPI) {
    cout << "\nEnter city: ";
    cin >> city;
}

displayTouristAttractions(formatStationName(city));
    }

    // =========================================================
    // 🔥 CASE 6 — NEARBY STATIONS
    // =========================================================
else if (choice == 6) {

    string source = isAPI ? src : "";
    string destination = isAPI ? dest : "";

    if (!isAPI) {
        cout << "Enter source station: ";
        cin >> source;

        cout << "Enter destination station: ";
        cin >> destination;
    }

    string url = getDirectionsUrl(source, destination);

    // 🔥 STRUCTURED OUTPUT FOR FRONTEND
    cout << "MAP_START\n";
    cout << "source:" << source << "\n";
    cout << "destination:" << destination << "\n";
    cout << "url:" << url << "\n";
    cout << "MAP_END\n";

    // CLI only
    if (!isAPI) {
        cout << "\n📍 URL: " << url << endl;
        cout << "Open in browser? (y/n): ";
        char c;
        cin >> c;

        if (c=='y' || c=='Y') openUrlInBrowser(url);
    } 
}

    // =========================================================
    // 🔥 CASE 7 — MAP / LOCATION
    // =========================================================
    else if (choice == 7) {

     //   cout << "\n==================== COMPLETE JOURNEY PLANNER ====================\n";

   
  //  string rawFrom = isAPI ? src : "";
//string rawTo   = isAPI ? dest : "";

string userFrom = src;
string userTo   = dest;

string rawFrom = isAPI ? (src + ", India") : "";
string rawTo   = isAPI ? (dest + ", India") : "";


if (!isAPI) {
  //  cout << "📍 From: ";
    cin >> rawFrom;

    //cout << "📍 To: ";
    cin >> rawTo;
}
  //  cout << "🔍 Finding nearest railway/bus place...\n";

    // --- 2) Geocode with Maps (get candidate POIs)
    vector<Station> fromPOI = findStationsByLocation(rawFrom);
    vector<Station> toPOI   = findStationsByLocation(rawTo);

    auto pickBestPOI = [&](const vector<Station>& v, const string& fb){
        return v.empty() ? fb : v.front().name;
    };

    string mapsFrom = pickBestPOI(fromPOI, rawFrom);
    string mapsTo   = pickBestPOI(toPOI, rawTo);

 //   cout << "📍 Google Maps suggests: " << mapsFrom << "\n";
   // cout << "   Using as: " << mapsFrom << "\n";
  //  cout << "📍 Google Maps suggests: " << mapsTo << "\n";
  //  cout << "   Using as: " << mapsTo << "\n";

    // --- 3) Resolve real nearest railway station objects
  //  cout << "\n📍 Finding nearest railway station to: " << rawFrom << "\n";
    Station realFromStation;
    {
     //   auto nearby = findStationsByLocation(rawFrom);
     auto nearby = fromPOI;
        if (!nearby.empty()) realFromStation = nearby.front();
        else { realFromStation.name = mapsFrom; realFromStation.address = ""; }
    }
///    cout << "✅ Found station: " << realFromStation.name << "\n";

  ///  cout << "📍 Finding nearest railway station to: " << rawTo << "\n";
    Station realToStation;
    {
        //auto nearby = findStationsByLocation(rawTo);
        auto nearby = toPOI; 
        if (!nearby.empty()) realToStation = nearby.front();
        else { realToStation.name = mapsTo; realToStation.address = ""; }
    }
 ///   cout << "✅ Found station: " << realToStation.name << "\n";

    // THESE are the correct hubs
    string bestFrom = realFromStation.name;
    string bestTo   = realToStation.name;

   // cout << "\n✨ USING FINAL NAMES FOR ROUTE SEARCH:\n";
    //cout << "👉 FROM: " << bestFrom << "\n";
    //cout << "👉 TO:   " << bestTo << "\n";

   // cout << "\n🧠 Gathering routes...\n";
   // auto directTrainOptions = findDirectTrains(bestFrom, bestTo);


// 🔥 USE ORIGINAL USER INPUT (NOT POI)
string srcCode = findStationCode(src);
string dstCode = findStationCode(dest);

// fallback
if (srcCode.empty()) srcCode = src;
if (dstCode.empty()) dstCode = dest;


//auto directTrainOptions = findDirectTrains(srcCode, dstCode);
auto directTrainOptions = findDirectTrains(
    findStationCode(userFrom),
    findStationCode(userTo)
);

    vector<JourneyStep> planDirect;
    vector<JourneyStep> planMixed;
    bool directMeaningful = false;

    if (!directTrainOptions.empty()) {
        // We have at least one direct train option -> build full direct plan
        directMeaningful = true;
        auto opt = directTrainOptions.front();

        // Build direct TRAIN step
        JourneyStep tstep;
        tstep.mode = "TRAIN";
        tstep.vehicle = opt.trainNumber + " - " + opt.trainName;
        tstep.from = opt.source;
        tstep.to   = opt.destination;

        // Compute proper duration (prefer depart/arrive then duration field then distance fallback)
        unsigned long dsec = 0;
        if (!opt.departure.empty() && !opt.arrival.empty()) {
            long dep = parseTimeHHMMSS(opt.departure);
            long arr = parseTimeHHMMSS(opt.arrival);
           // if (arr < dep) arr += 24 * 3600;
// 🔥 SMART DAY ROLLOVER FIX
if (arr < dep) {
    arr += 24 * 3600;
}

// 🔥 EXTRA FIX for long distance trains
if ((opt.distance > 1200 || opt.distanceKm > 1200) && (arr - dep) < 6 * 3600) {
    arr += 24 * 3600;
}

            dsec = (unsigned long)max(0L, arr - dep);
        }
        if (dsec == 0 && opt.duration > 0) dsec = (unsigned long)round(opt.duration * 60.0);
        if (dsec == 0) dsec = (unsigned long)round((opt.distance > 0 ? opt.distance : 100.0) / 50.0 * 3600.0);

        tstep.durationSec = dsec;
        tstep.distanceKm = (opt.distance > 0 ? opt.distance : (opt.distanceKm>0?opt.distanceKm:0.0));
        tstep.departTime = opt.departure;
        tstep.arriveTime = opt.arrival;
        tstep.instruction = "Board " + tstep.vehicle;

        // FIRST MILE: compute real distance/time from rawFrom -> station
      //  DistanceMatrixResult fm = guardedDistanceMatrix(rawFrom, bestFrom);
        //if (!fm.ok) fm = estimateFallback(3.0);
DistanceMatrixResult fm, lm;

if (!isAPI) {
  //  fm = guardedDistanceMatrix(rawFrom, bestFrom);
    //if (!fm.ok) fm = estimateFallback(3.0);

    fm = guardedDistanceMatrix(rawFrom, bestFrom);

// 🔥 HARD LIMIT FIX
if (!fm.ok || fm.distanceKm > 100) {
    fm = estimateFallback(3.0);
}

  //  lm = guardedDistanceMatrix(bestTo, rawTo);
    //if (!lm.ok) lm = estimateFallback(3.0);

lm = guardedDistanceMatrix(bestTo, rawTo);

if (!lm.ok || lm.distanceKm > 100) {
    lm = estimateFallback(3.0);
}

} else {
    fm = estimateFallback(3.0);
    lm = estimateFallback(3.0);
}


        JourneyStep d_first = journey_planner::buildLocalStep(rawFrom, bestFrom, fm.durationSec, fm.distanceKm, "First-mile to station");

        // LAST MILE: compute real distance/time from station -> rawTo
        //DistanceMatrixResult lm = guardedDistanceMatrix(bestTo, rawTo);
        //if (!lm.ok) lm = estimateFallback(3.0);

        JourneyStep d_last = journey_planner::buildLocalStep(bestTo, rawTo, lm.durationSec, lm.distanceKm, "Last-mile to destination");

        // Compose direct plan (first-mile, train, last-mile)
        if (d_first.durationSec > 0 || d_first.distanceKm > 0.0) planDirect.push_back(d_first);
        planDirect.push_back(tstep);
        if (d_last.durationSec > 0 || d_last.distanceKm > 0.0) planDirect.push_back(d_last);
    }

    // --- 5) Build AI-MIX fallback plan only if we don't have a meaningful direct plan
    if (!directMeaningful) {
     //   cout << "❌ No direct train/bus found within 50 km.\n";
      //  cout << "👉 Please choose AI-Mixed Route (Recommended).\n\n";
        planMixed = journey_planner::getRealTimeJourneyPlan(rawFrom, rawTo, 1);

    } else {
        planMixed = journey_planner::getRealTimeJourneyPlan(rawFrom, rawTo, 1);
    }
    

    if (planMixed.empty()) {
//    cout << "⚠️ Could not fetch live route. Showing fallback.\n";
}

    // --- 6) Present summary and let user choose
 //   cout << "\n============================================================\n";
  //  cout << "🌐 ROUTE SUMMARY\n";
    //cout << "From: " << bestFrom << "  →  To: " << bestTo << "\n";
    //cout << "============================================================\n\n";



// ================= API OUTPUT (IMPORTANT) =================

//vector<JourneyStep> finalPlan;
/*
// 🔥 AUTO DECISION
if (!directMeaningful || planDirect.empty()) {
    finalPlan = planMixed;

    cout << "ROUTE_MODE:AI_ONLY\n";
    cout << "INFO:Direct route not available\n";
}
else {
    finalPlan = planDirect;

    cout << "ROUTE_MODE:DIRECT\n";
}
*/

// 🔥 NEW INTELLIGENT MODE

bool hasDirect = (directMeaningful && !planDirect.empty());
bool hasAI = (!planMixed.empty());

if (hasDirect && hasAI) {
    cout << "ROUTE_MODE:BOTH\n";
}
else if (hasDirect) {
    cout << "ROUTE_MODE:DIRECT_ONLY\n";
}
else {
    cout << "ROUTE_MODE:AI_ONLY\n";
}

// ================= CONVERT TO JourneyPlan =================

//JourneyPlan apiPlan;

JourneyPlan directAPI;
JourneyPlan aiAPI;

auto buildAPI = [&](vector<JourneyStep> &plan, JourneyPlan &api) {

    long totalSec = 0;
    double totalKm = 0;

    for (auto &s : plan) {

        // 🔥 SAFETY FIX
        if (s.mode == "LOCAL" && (s.distanceKm > 50 || s.durationSec > 7200)) {
            s.distanceKm = 3.0;
            s.durationSec = 600;
        }

        PlannerStep ps;

        ps.mode = s.mode;
        ps.service = s.vehicle;
        ps.from = s.from;
        ps.to = s.to;

        if (!s.departTime.empty() && !s.arriveTime.empty()) {
            ps.time = s.departTime + " → " + s.arriveTime;
        } else {
            ps.time = to_string(s.durationSec/60) + " min";
        }

        ps.duration = to_string(s.durationSec/3600) + "h " +
                      to_string((s.durationSec%3600)/60) + "m";

        ostringstream d;
        d << fixed << setprecision(1) << s.distanceKm << " km";
        ps.distance = d.str();

        ps.note = s.instruction;

        api.steps.push_back(ps);

        totalSec += s.durationSec;
        totalKm += s.distanceKm;
    }

    api.totalTime = to_string(totalSec/3600) + "h " +
                    to_string((totalSec%3600)/60) + "m";

    ostringstream dk;
    dk << fixed << setprecision(1) << totalKm << " km";
    api.totalDistance = dk.str();
};

if (hasDirect) buildAPI(planDirect, directAPI);
if (hasAI) buildAPI(planMixed, aiAPI);



// 🔥 FINAL OUTPUT
//journey_planner::displayPlannerAPI(apiPlan);

if (hasDirect) {
    cout << "PLAN_TYPE:DIRECT\n";
    journey_planner::displayPlannerAPI(directAPI);
}

if (hasAI) {
    cout << "PLAN_TYPE:AI\n";
    journey_planner::displayPlannerAPI(aiAPI);
}

    }

    // =========================================================
    // 🔥 CASE 8 — FLOYD WARSHALL (SMART ROUTE)
    // =========================================================
    else if (choice == 8) {

           cout << "\n🔁 INDIRECT TRAIN ROUTES (INTERMEDIATE STATIONS)\n";

  //  string source, destination, srcCode, dstCode;

    // SOURCE
string source = isAPI ? src : "";

if (!isAPI) {
    cout << "Enter source station: ";
    cin >> source;
}

string srcCode = findStationCode(source);

if (srcCode.empty()) {
    cout << "❌ Station not found.\n";
    return 0;
}

cout << "✅ Station identified: "
     << getStationCity(srcCode) << " (" << srcCode << ")\n";


    // DESTINATION
string destination = isAPI ? dest : "";

if (!isAPI) {
    cout << "Enter destination station: ";
    cin >> destination;
}

string dstCode = findStationCode(destination);

if (dstCode.empty()) {
    cout << "❌ Station not found.\n";
    return 0;
}

cout << "✅ Station identified: "
     << getStationCity(dstCode) << " (" << dstCode << ")\n";

    // overwrite with station codes
    source = srcCode;
    destination = dstCode;

    // DIRECTLY SHOW INDIRECT ROUTES
    handleIndirectRoutes(source, destination);
    //handleIndirectRoutes(srcCode, dstCode);

    }

    // =========================================================
    // ❌ INVALID OPTION
    // =========================================================
    else {
        cout << "INVALID_OPTION\n";
    }

    cout << "========================================\n";

    return 0;
}



 