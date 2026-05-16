#include "maps_api.h"
#include <iostream>
#include <sstream>
#include <curl/curl.h>
#include <json/json.h>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <thread>
#include <climits>
#include <chrono>
#include <ctime>    // time_t
#include <iomanip>  // put_time
#include <sstream>
using namespace std;

// Your Google Maps API Key - Replace with your actual key
const string GOOGLE_MAPS_API_KEY = "YOUR_API_KEY";

/*
struct Station {
    std::string name;
    std::string address;
    double lat, lng;
};
*/

// Callback function for writing HTTP response
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* response) {
    size_t totalSize = size * nmemb;
    response->append((char*)contents, totalSize);
    return totalSize;
}
static unsigned long safeParseDurationSeconds(const Json::Value &leg) {
    if (leg.isMember("duration") && leg["duration"].isMember("value"))
        return (unsigned long)leg["duration"]["value"].asLargestUInt();
    return 0;
}

// Function to URL encode a string
string urlEncode(const string& str) {
    string encoded;
    char hex[4];
    
    for (char c : str) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        } else if (c == ' ') {
            encoded += '+';
        } else {
            sprintf(hex, "%%%02X", (unsigned char)c);
            encoded += hex;
        }
    }
    return encoded;
}

// Function to make HTTP request to Google Maps API
string makeGoogleMapsRequest(const string& url) {
    CURL* curl;
    CURLcode res;
    string response;

    curl = curl_easy_init();
    if (!curl) {
        cerr << "curl init failed" << endl;
        return response;
    }

    // Common 
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "IndianRailwaysTouristGuide/1.0");

    // TLS & CA - helps on some systems
   // curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
    // If you have a CA bundle, set it (uncomment and set path):
    // curl_easy_setopt(curl, CURLOPT_CAINFO, "/etc/ssl/certs/ca-certificates.crt");

    // Timeouts
curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);


// curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
curl_easy_setopt(curl, CURLOPT_CAINFO, "/etc/ssl/certs/ca-certificates.crt");

  //  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L); // 15s connect
    //curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);        // 30s overall

    // Automatic retries for transient errors
    const int MAX_RETRIES = 3;
    int attempt = 0;
    while (attempt < MAX_RETRIES) {
        response.clear();
        res = curl_easy_perform(curl);
        if (res == CURLE_OK) break;

        // Print only once per attempt
        cerr << "curl_easy_perform() failed (attempt " << (attempt+1) << "): " 
             << curl_easy_strerror(res) << endl;

        // Small backoff
        attempt++;
        std::this_thread::sleep_for(std::chrono::seconds(1 + attempt));
    }

    curl_easy_cleanup(curl);
    return response;
}


// Function to calculate distance between two coordinates (Haversine formula)
double calculateDistance(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371.0; // Earth radius in kilometers
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;
    
    double a = sin(dLat/2) * sin(dLat/2) +
               cos(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0) *
               sin(dLon/2) * sin(dLon/2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    
    return R * c;
}

Location geocodeAddress(const string& address) {
    Location location;
    location.latitude = 0;
    location.longitude = 0;

    string encodedAddress = urlEncode(address);
    string url = "https://maps.googleapis.com/maps/api/geocode/json?address=" +
                 encodedAddress + "&key=" + GOOGLE_MAPS_API_KEY;

    string response = makeGoogleMapsRequest(url);

    Json::CharReaderBuilder reader;
    Json::Value root;
    string errors;
    istringstream responseStream(response);

    if (!Json::parseFromStream(reader, responseStream, &root, &errors)) {
        cout << "❌ Failed to parse geocoding JSON\n";
        return location;
    }

    string status = root["status"].asString();
    if (status != "OK") {
        cout << "❌ Geocoding error: " << status << endl;
        return location;
    }

    Json::Value result = root["results"][0];

    if (result.isMember("geometry") &&
        result["geometry"].isMember("location"))
    {
        location.latitude =
            result["geometry"]["location"]["lat"].asDouble();
        location.longitude =
            result["geometry"]["location"]["lng"].asDouble();

        cout << "📍 Geocoded: " << address
             << " → (" << location.latitude
             << ", " << location.longitude << ")\n";
    }

    return location;
}

vector<Station> findNearbyStations(double latitude, double longitude, double radius) {
    vector<Station> stations;

    stringstream url;
    url << "https://maps.googleapis.com/maps/api/place/nearbysearch/json?"
        << "location=" << latitude << "," << longitude
        << "&radius=" << (radius * 1000)
        << "&type=train_station"
        << "&key=" << GOOGLE_MAPS_API_KEY;

    string response = makeGoogleMapsRequest(url.str());

    Json::CharReaderBuilder reader;
    Json::Value root;
    string errors;
    istringstream responseStream(response);

    if (!Json::parseFromStream(reader, responseStream, &root, &errors)) {
        cout << "❌ Failed to parse Places API JSON" << endl;
        return stations;
    }

    if (root["status"].asString() != "OK") {
        cout << "❌ Google returned error: " << root["status"].asString() << endl;
        return stations;
    }

    for (auto &place : root["results"]) {
        Station s;
        s.name = place["name"].asString();
        s.address = place["vicinity"].asString();
        s.latitude = place["geometry"]["location"]["lat"].asDouble();
        s.longitude = place["geometry"]["location"]["lng"].asDouble();
        s.distance = calculateDistance(latitude, longitude, s.latitude, s.longitude);
        stations.push_back(s);
    }

    sort(stations.begin(), stations.end(),
         [](const Station &a, const Station &b) {
             return a.distance < b.distance;
         });

    return stations;
}
vector<Station> findStationsByLocation(const string& location) {
    cout << "🗺️ Geocoding location: " << location << endl;

    Location userLoc = geocodeAddress(location);
    if (userLoc.latitude == 0 && userLoc.longitude == 0) {
        cout << "❌ Geocoding failed for " << location << endl;
        return {};
    }

    vector<double> radii = {10, 20, 50, 70}; // up to 70km
    vector<Station> results;

    for (double r : radii) {
        vector<Station> nearby = findNearbyStations(userLoc.latitude,
                                                    userLoc.longitude,
                                                    r);
        for (auto &s : nearby)
            results.push_back(s);

        if (results.size() >= 5) break;
    }

    if (results.size() > 5)
        results.resize(5);

    return results;
}



// ==========================================================
//  ⭐ Fetch REAL ATTRACTIONS along the route
// ==========================================================
// We sample 3 coordinates along the route polyline and search for POIs around them.
vector<string> getAttractionsAlongRoute(const string& origin,
                                        const string& destination,
                                        int count)
{
    vector<string> attractions;
    if (count <= 0) return attractions;

    // 1. Call Google Directions (driving) to get route path
    auto routes = findDirections(origin, destination, "driving");
    if (routes.empty()) return attractions;

    // We find 3 sampling points:
    // start, mid, end of route
    string encodedOrigin = urlEncode(origin);
    string encodedDestination = urlEncode(destination);

    string url = "https://maps.googleapis.com/maps/api/directions/json"
                 "?origin=" + encodedOrigin +
                 "&destination=" + encodedDestination +
                 "&mode=driving&key=" + GOOGLE_MAPS_API_KEY;

    string response = makeGoogleMapsRequest(url);

    Json::CharReaderBuilder reader;
    Json::Value root;
    string errs;
    istringstream ss(response);
    if (!Json::parseFromStream(reader, ss, &root, &errs)) {
        return attractions;
    }

    if (root["status"].asString() != "OK") return attractions;

    // Get polyline points
    if (!root["routes"][0].isMember("overview_polyline")) return attractions;

    string encodedPolyline = root["routes"][0]["overview_polyline"]["points"].asString();

    // decode polyline
    auto decodePolyline = [](const string &polyline) {
        vector<pair<double,double>> points;
        int index = 0, len = polyline.length();
        int lat = 0, lng = 0;

        while (index < len) {
            int b, shift = 0, result = 0;
            do {
                b = polyline[index++] - 63;
                result |= (b & 0x1f) << shift;
                shift += 5;
            } while (b >= 0x20);
            lat += ((result & 1) ? ~(result >> 1) : (result >> 1));

            shift = 0;
            result = 0;
            do {
                b = polyline[index++] - 63;
                result |= (b & 0x1f) << shift;
                shift += 5;
            } while (b >= 0x20);
            lng += ((result & 1) ? ~(result >> 1) : (result >> 1));

            points.push_back({ lat * 1e-5, lng * 1e-5 });
        }
        return points;
    };

    vector<pair<double,double>> pts = decodePolyline(encodedPolyline);
    if (pts.empty()) return attractions;

    // sample: start, mid, end
    vector<pair<double,double>> samplePts;
    samplePts.push_back(pts.front());
    samplePts.push_back(pts[pts.size()/2]);
    samplePts.push_back(pts.back());

    // Google Places API search
    for (auto &p : samplePts) {
        if ((int)attractions.size() >= count) break;

        ostringstream url2;
        url2 << "https://maps.googleapis.com/maps/api/place/nearbysearch/json?"
             << "location=" << p.first << "," << p.second
             << "&radius=5000&type=tourist_attraction&key=" << GOOGLE_MAPS_API_KEY;

        string resp2 = makeGoogleMapsRequest(url2.str());

        Json::Value root2;
        istringstream ss2(resp2);
        if (!Json::parseFromStream(reader, ss2, &root2, &errs)) continue;

        for (auto &pl : root2["results"]) {
            if ((int)attractions.size() >= count) break;
            if (pl.isMember("name")) attractions.push_back(pl["name"].asString());
        }
    }

    return attractions;
}



// NEW FUNCTION: Find nearest railway station using Google Maps Places API
string findNearestStation(const string& location) {
    cout << "📍 Finding nearest railway station to: " << location << endl;
    
    // Use Find Place From Text API
    string encodedLocation = urlEncode(location + " railway station");
    string url = "https://maps.googleapis.com/maps/api/place/findplacefromtext/json"
                 "?input=" + encodedLocation +
                 "&inputtype=textquery" +
                 "&fields=name,formatted_address,geometry" +
                 "&key=" + GOOGLE_MAPS_API_KEY;
    
    string response = makeGoogleMapsRequest(url);
    
    // Parse JSON response
    Json::CharReaderBuilder reader;
    Json::Value root;
    string errors;
    istringstream responseStream(response);
    
    if (Json::parseFromStream(reader, responseStream, &root, &errors)) {
        string status = root["status"].asString();
        
        if (status == "OK" && root["candidates"].size() > 0) {
            string stationName = root["candidates"][0]["name"].asString();
            cout << "✅ Found station: " << stationName << endl;
            
            // Get additional details if available
            if (root["candidates"][0].isMember("formatted_address")) {
                string address = root["candidates"][0]["formatted_address"].asString();
                cout << "   📍 Address: " << address << endl;
            }
            
            return stationName;
        } else {
            cout << "❌ No station found for: " << location << endl;
            cout << "💡 Using location as station name" << endl;
            return location;
        }
    } else {
        cout << "❌ Error parsing Google Maps response" << endl;
        return location; // Fallback to original input
    }
}

// NEW FUNCTION: Get detailed station information
Station getStationDetails(const string& stationName) {
    Station station;
    station.trainName = stationName;
    
    // Use Geocoding API to get detailed information
    string encodedStation = urlEncode(stationName + " railway station India");
    string url = "https://maps.googleapis.com/maps/api/geocode/json?address=" +
                 encodedStation + "&key=" + GOOGLE_MAPS_API_KEY;
    
    string response = makeGoogleMapsRequest(url);
    
    // Parse JSON response
    Json::CharReaderBuilder reader;
    Json::Value root;
    string errors;
    istringstream responseStream(response);
    
    if (Json::parseFromStream(reader, responseStream, &root, &errors)) {
        if (root["status"].asString() == "OK" && root["results"].size() > 0) {
            Json::Value result = root["results"][0];
            
            // Get coordinates
            if (result.isMember("geometry") && result["geometry"].isMember("location")) {
                station.latitude = result["geometry"]["location"]["lat"].asDouble();
                station.longitude = result["geometry"]["location"]["lng"].asDouble();
            }
            
            // Get address
            if (result.isMember("formatted_address")) {
                station.address = result["formatted_address"].asString();
            } else {
                station.address = stationName;
            }
            
            cout << "📍 Station details: " << station.trainName << " at " << station.address << endl;
        }
    }
    
    return station;
}

// Function to get train routes using Google Maps Directions API
// Function to get train routes using Google Maps Directions API
vector<TrainRoute> findTrainRoutes(const string& origin, const string& destination) {
    vector<TrainRoute> routes;
    
    cout << "🚆 Getting real train routes from Google Maps..." << endl;
    
    string encodedOrigin = urlEncode(origin);
    string encodedDestination = urlEncode(destination);
    
    string url = "https://maps.googleapis.com/maps/api/directions/json"
                 "?origin=" + encodedOrigin +
                 "&destination=" + encodedDestination +
                 "&mode=transit" +
                 "&transit_mode=train" +
                 "&alternatives=true" +
                 "&key=" + GOOGLE_MAPS_API_KEY;
    
    string response = makeGoogleMapsRequest(url);
    
    // Parse JSON response
    Json::CharReaderBuilder reader;
    Json::Value root;
    string errors;
    istringstream responseStream(response);
    
    if (Json::parseFromStream(reader, responseStream, &root, &errors)) {
        string status = root["status"].asString();
        
        if (status == "OK") {
            cout << "✅ Google Maps found " << root["routes"].size() << " route(s)" << endl;
            
            for (const auto& route : root["routes"]) {
                TrainRoute trainRoute;
                
                // Get route overview - WITH PROPER ERROR CHECKING
                if (route.isMember("legs") && route["legs"].size() > 0) {
                    Json::Value leg = route["legs"][0];
                    
                    // SAFE: Get distance and duration with fallbacks
                    if (leg.isMember("distance") && leg["distance"].isMember("text")) {
                        trainRoute.distance = leg["distance"]["text"].asString();
                    } else {
                        trainRoute.distance = "500 km"; // Default fallback
                    }
                    
                    if (leg.isMember("duration") && leg["duration"].isMember("text")) {
                        trainRoute.duration = leg["duration"]["text"].asString();
                    } else {
                        trainRoute.duration = "8 hours"; // Default fallback
                    }
                    
                    // SAFE: Parse steps to find train information
                    if (leg.isMember("steps")) {
                        for (const auto& step : leg["steps"]) {
                            if (step.isMember("travel_mode") && step["travel_mode"].asString() == "TRANSIT" &&
                                step.isMember("transit_details")) {
                                Json::Value transit = step["transit_details"];
                                
                                TrainInfo train;
                                
                                // SAFE: Get train info with fallbacks
                                if (transit.isMember("line") && transit["line"].isMember("name")) {
                                    train.trainName = transit["line"]["name"].asString();
                                } else {
                                    train.trainName = "Express Train";
                                }
                                
                                if (transit.isMember("departure_stop") && transit["departure_stop"].isMember("name")) {
                                    train.departureStation = transit["departure_stop"]["name"].asString();
                                } else {
                                    train.departureStation = "Departure Station";
                                }
                                
                                if (transit.isMember("arrival_stop") && transit["arrival_stop"].isMember("name")) {
                                    train.arrivalStation = transit["arrival_stop"]["name"].asString();
                                } else {
                                    train.arrivalStation = "Arrival Station";
                                }
                                
                                if (transit.isMember("departure_time") && transit["departure_time"].isMember("text")) {
                                    train.departureTime = transit["departure_time"]["text"].asString();
                                } else {
                                    train.departureTime = "Check schedule";
                                }
                                
                                if (transit.isMember("arrival_time") && transit["arrival_time"].isMember("text")) {
                                    train.arrivalTime = transit["arrival_time"]["text"].asString();
                                } else {
                                    train.arrivalTime = "Check schedule";
                                }
                                
                                if (step.isMember("duration") && step["duration"].isMember("text")) {
                                    train.duration = step["duration"]["text"].asString();
                                } else {
                                    train.duration = "4 hours";
                                }
                                
                                if (step.isMember("distance") && step["distance"].isMember("text")) {
                                    train.distance = step["distance"]["text"].asString();
                                } else {
                                    train.distance = "400 km";
                                }
                                
                                trainRoute.trains.push_back(train);
                            }
                        }
                    }
                } else {
                    // If no legs found, create a default route
                    trainRoute.distance = "500 km";
                    trainRoute.duration = "8 hours";
                    cout << "⚠️  No route legs found, using default" << endl;
                }
                
                routes.push_back(trainRoute);
            }
        } else {
            cout << "❌ Google Maps Directions API error: " << status << endl;
            cout << "💡 Creating fallback route..." << endl;
            
            // CREATE FALLBACK ROUTE - This prevents segmentation fault!
            TrainRoute fallbackRoute;
            fallbackRoute.distance = "600 km";
            fallbackRoute.duration = "10 hours";
            routes.push_back(fallbackRoute);
        }
    } else {
        cout << "❌ Failed to parse Google Maps JSON response" << endl;
        cout << "💡 Creating guaranteed fallback route..." << endl;
        
        // CREATE GUARANTEED FALLBACK ROUTE
        TrainRoute guaranteedRoute;
        guaranteedRoute.distance = "700 km";
        guaranteedRoute.duration = "12 hours";
        routes.push_back(guaranteedRoute);
    }
    
    // GUARANTEE: Always return at least one route
    if (routes.empty()) {
        cout << "🔄 No routes found, creating default route..." << endl;
        TrainRoute defaultRoute;
        defaultRoute.distance = "800 km";
        defaultRoute.duration = "15 hours";
        routes.push_back(defaultRoute);
    }
    
    return routes;
}
// Add to maps_api.cpp (requires the Json parsing code and WriteCallback + urlEncode + makeGoogleMapsRequest existing in this file)

// Helper: parse ISO-like durations returned as text "2 hours 10 mins" -> seconds (approx)
unsigned long parseDurationTextToSeconds(const std::string &durText){

//static long parseDurationTextToSeconds(const std::string &durText) {
    // tries to parse patterns like "1 day 3 hours 20 mins", "2 hours 10 mins", "45 mins"
    long totalMin = 0;
    std::string s = durText;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);

    // Parse days
    size_t pos = s.find("day");
    if (pos != std::string::npos) {
        // extract integer before 'day'
        int val = 0;
        sscanf(s.c_str(), "%d", &val);
        totalMin += val * 24 * 60;
    }
    // hours
    pos = s.find("hour");
    if (pos != std::string::npos) {
        int h = 0;
        // find number preceding 'hour'
        const char *c = s.c_str();
        // rough parse: look backwards for number
        for (int i = pos-1; i >= 0; --i) {
            if (isdigit(c[i])) {
                // find start
                int j = i;
                while (j-1 >=0 && isdigit(c[j-1])) j--;
                std::string sub = std::string(c+j, c+i+1);
                h = atoi(sub.c_str());
                break;
            }
        }
        totalMin += h * 60;
    }
    // minutes
    pos = s.find("min");
    if (pos != std::string::npos) {
        int m = 0;
        const char *c = s.c_str();
        for (int i = pos-1; i >= 0; --i) {
            if (isdigit(c[i])) {
                int j = i;
                while (j-1 >=0 && isdigit(c[j-1])) j--;
                std::string sub = std::string(c+j, c+i+1);
                m = atoi(sub.c_str());
                break;
            }
        }
        totalMin += m;
    }
    return totalMin * 60; // seconds
}

// Generic directions function that supports transit/driving/walking
std::vector<TrainRoute> findDirections(const std::string& origin, const std::string& destination, const std::string& mode) {
    std::vector<TrainRoute> routes;
    std::string encodedOrigin = urlEncode(origin);
    std::string encodedDestination = urlEncode(destination);

    std::string url = "https://maps.googleapis.com/maps/api/directions/json"
                      "?origin=" + encodedOrigin +
                      "&destination=" + encodedDestination +
                      "&alternatives=true" +
                      "&key=" + GOOGLE_MAPS_API_KEY;

    if (!mode.empty()) {
        url += "&mode=" + urlEncode(mode);
    } else {
        url += "&mode=transit"; // default to transit (includes bus/train)
    }

    // If we specifically want transit and not restrict to trains:
    // do not set transit_mode; allow Google to return bus/train/metro.
    std::string response = makeGoogleMapsRequest(url);

    Json::CharReaderBuilder reader;
    Json::Value root;
    std::string errs;
    std::istringstream ss(response);
    if (!Json::parseFromStream(reader, ss, &root, &errs)) {
        std::cerr << "❌ Failed to parse Directions JSON" << std::endl;
        return routes;
    }
    std::string status = root.get("status", "").asString();
    if (status != "OK") {
        // no routes
        return routes;
    }

    for (const auto& r : root["routes"]) {
        TrainRoute tr;
        tr.trains.clear();
        tr.distance = r["legs"][0]["distance"]["text"].asString();
        tr.duration = r["legs"][0]["duration"]["text"].asString();
        // total duration in seconds (use google numeric if present)
        if (r["legs"].size() > 0 && r["legs"][0].isMember("duration") && r["legs"][0]["duration"].isMember("value")) {
            // numeric seconds
            tr.duration_seconds = r["legs"][0]["duration"]["value"].asLargestUInt();
        } else {
            tr.duration_seconds = parseDurationTextToSeconds(tr.duration);
        }

        // collect transit steps from the first leg
        if (r["legs"].size() > 0) {
            const Json::Value &leg = r["legs"][0];
            if (leg.isMember("steps")) {
                for (const auto &step : leg["steps"]) {
                    if (step.isMember("travel_mode") && step["travel_mode"].asString() == "TRANSIT") {
    TrainInfo ti;
    ti.trainName = step["transit_details"]["line"].get("name", "").asString();
    ti.departureStation = step["transit_details"]["departure_stop"].get("name", "").asString();
    ti.arrivalStation   = step["transit_details"]["arrival_stop"].get("name", "").asString();
    ti.departureTime    = step["transit_details"]["departure_time"].get("text", "").asString();
    ti.arrivalTime      = step["transit_details"]["arrival_time"].get("text", "").asString();
    ti.duration         = step["duration"].get("text", "").asString();
    ti.distance         = step["distance"].get("text", "").asString();

    // vehicle type detection: prefer "type" then "name"
    if (step["transit_details"]["line"].isMember("vehicle") &&
        step["transit_details"]["line"]["vehicle"].isMember("type")) {
        ti.vehicleType = step["transit_details"]["line"]["vehicle"]["type"].asString(); // e.g., "TRAIN" or "BUS"
    } else if (step["transit_details"]["line"].isMember("vehicle") &&
               step["transit_details"]["line"]["vehicle"].isMember("name")) {
        ti.vehicleType = step["transit_details"]["line"]["vehicle"]["name"].asString();
    } else {
        // fallback to inspect line name for 'express' / 'rail' strings
        std::string ln = ti.trainName;
        std::transform(ln.begin(), ln.end(), ln.begin(), ::tolower);
        if (ln.find("express") != std::string::npos || ln.find("rail") != std::string::npos ||
            ln.find("passenger") != std::string::npos) {
            ti.vehicleType = "TRAIN";
        } else {
            ti.vehicleType = "TRANSIT";
        }
    }

    if (ti.vehicleType.find("TRAIN") != std::string::npos || ti.vehicleType.find("rail") != std::string::npos) {
        tr.hasTrain = true;
    }
    tr.trains.push_back(ti);
}

                    /*
                    if (step.isMember("travel_mode") && step["travel_mode"].asString() == "TRANSIT") {
                        TrainInfo ti;
                        ti.trainName = step["transit_details"]["line"].get("name", "").asString();
                        ti.departureStation = step["transit_details"]["departure_stop"].get("name", "").asString();
                        ti.arrivalStation   = step["transit_details"]["arrival_stop"].get("name", "").asString();
                        ti.departureTime    = step["transit_details"]["departure_time"].get("text", "").asString();
                        ti.arrivalTime      = step["transit_details"]["arrival_time"].get("text", "").asString();
                        ti.duration         = step["duration"].get("text", "").asString();
                        ti.distance         = step["distance"].get("text", "").asString();
                        // also save vehicle type (if any)
                        if (step["transit_details"].isMember("line") && step["transit_details"]["line"].isMember("vehicle")) {
                            ti.vehicleType = step["transit_details"]["line"]["vehicle"].get("name", "").asString();
                        }
                        tr.trains.push_back(ti);
                    }
                    */
                }
            }
        }
        routes.push_back(tr);
    }
    return routes;
}

std::vector<TrainRoute> findTransitRoutes(const std::string &origin, const std::string &destination) {
    std::vector<TrainRoute> routes;
    std::string encodedOrigin = urlEncode(origin + " railway station");
    std::string encodedDestination = urlEncode(destination + " railway station");

    // Use departure_time=now to get next upcoming trains
    long departureEpoch = (long)time(nullptr);
    std::ostringstream url;
    url << "https://maps.googleapis.com/maps/api/directions/json"
        << "?origin=" << encodedOrigin
        << "&destination=" << encodedDestination
        << "&mode=transit"
        << "&transit_mode=train"
        << "&alternatives=true"
        << "&departure_time=" << departureEpoch
        << "&key=" << GOOGLE_MAPS_API_KEY;

    std::string response = makeGoogleMapsRequest(url.str());

    Json::CharReaderBuilder reader;
    Json::Value root;
    std::string errs;
    std::istringstream s(response);
    if (!Json::parseFromStream(reader, s, &root, &errs)) {
        cerr << "❌ Failed to parse Directions JSON" << endl;
        return routes;
    }

    if (!root.isMember("status") || root["status"].asString() != "OK") {
        cerr << "❌ Directions API status: " << root["status"].asString() << endl;
        return routes;
    }

    for (const auto &routeJson : root["routes"]) {
        TrainRoute tr;
        // total duration/distance from combined legs
        unsigned long durationSecondsTotal = 0;
        std::string distText = "";
        std::string durationText = "";

        if (routeJson.isMember("legs") && routeJson["legs"].size() > 0) {
            const Json::Value &leg0 = routeJson["legs"][0];
            if (leg0.isMember("duration") && leg0["duration"].isMember("text"))
                durationText = leg0["duration"]["text"].asString();
            if (leg0.isMember("distance") && leg0["distance"].isMember("text"))
                distText = leg0["distance"]["text"].asString();
            durationSecondsTotal = safeParseDurationSeconds(leg0);
            // if durationSecondsTotal is zero, we will sum step durations later
        }

        tr.distance = distText;
        tr.duration = durationText;
        tr.duration_seconds = durationSecondsTotal;

        // parse steps and find transit(train) steps
        if (routeJson.isMember("legs") && routeJson["legs"].size() > 0) {
            const Json::Value &leg = routeJson["legs"][0];
            if (leg.isMember("steps")) {
                for (const auto &step : leg["steps"]) {
                    if (step.isMember("travel_mode") && step["travel_mode"].asString() == "TRANSIT") {
                        TrainInfo ti;
                        const Json::Value &td = step["transit_details"];
                        // line info may be in td["line"]
                        if (td.isMember("line")) {
                            const Json::Value &line = td["line"];
                            if (line.isMember("name")) ti.trainName = line["name"].asString();
                            if (line.isMember("short_name")) ti.short_name = line["short_name"].asString();
                            if (line.isMember("vehicle")) {
                                const Json::Value &vehicle = line["vehicle"];
                                if (vehicle.isMember("type")) ti.vehicleType = vehicle["type"].asString();
                                else if (vehicle.isMember("name")) ti.vehicleType = vehicle["name"].asString();
                            }
                            // sometimes agencies available
                            if (line.isMember("agencies") && line["agencies"].isArray() && line["agencies"].size() > 0) {
                                if (line["agencies"][0].isMember("name")) ti.agency = line["agencies"][0]["name"].asString();
                            }
                        }
                        if (td.isMember("departure_stop") && td["departure_stop"].isMember("name"))
                            ti.departureStation = td["departure_stop"]["name"].asString();
                        if (td.isMember("arrival_stop") && td["arrival_stop"].isMember("name"))
                            ti.arrivalStation = td["arrival_stop"]["name"].asString();
                        if (td.isMember("departure_time")) {
                            if (td["departure_time"].isMember("value")) {
                                ti.departureTimeEpoch = (long)td["departure_time"]["value"].asLargestUInt();
                                // also a human text
                                if (td["departure_time"].isMember("text")) ti.departureTimeText = td["departure_time"]["text"].asString();
                            } else if (td["departure_time"].isMember("text")) {
                                ti.departureTimeText = td["departure_time"]["text"].asString();
                            }
                        }
                        if (td.isMember("arrival_time")) {
                            if (td["arrival_time"].isMember("value")) {
                                ti.arrivalTimeEpoch = (long)td["arrival_time"]["value"].asLargestUInt();
                                if (td["arrival_time"].isMember("text")) ti.arrivalTimeText = td["arrival_time"]["text"].asString();
                            } else if (td["arrival_time"].isMember("text")) {
                                ti.arrivalTimeText = td["arrival_time"]["text"].asString();
                            }
                        }
                        // step-level distance/duration fallback
                        if (step.isMember("duration") && step["duration"].isMember("text"))
                            ti.duration = step["duration"]["text"].asString();
                        if (step.isMember("distance") && step["distance"].isMember("text"))
                            ti.distance = step["distance"]["text"].asString();

                        // headsign if present
                        if (td.isMember("headsign")) ti.headsign = td["headsign"].asString();

                        // mark this route as having a train
                        std::string vtypeLower = ti.vehicleType;
                        std::transform(vtypeLower.begin(), vtypeLower.end(), vtypeLower.begin(), ::tolower);
                        if (vtypeLower.find("train") != std::string::npos || vtypeLower.find("rail") != std::string::npos ||
                            ti.trainName.find("Express") != std::string::npos || ti.trainName.find("SF") != std::string::npos ||
                            ti.trainName.find("Mail") != std::string::npos) {
                            tr.hasTrain = true;
                        }

                        tr.trains.push_back(ti);

                        // if route-level duration missing, add step duration to total
                        if (tr.duration_seconds == 0 && step.isMember("duration") && step["duration"].isMember("value")) {
                            durationSecondsTotal += step["duration"]["value"].asLargestUInt();
                            tr.duration_seconds = durationSecondsTotal;
                        }
                    }
                }
            }
        }

        // Safety: if still zero, compute from route overview (sum legs durations)
        if (tr.duration_seconds == 0 && routeJson.isMember("legs")) {
            unsigned long sumSec = 0;
            for (const auto &leg : routeJson["legs"]) sumSec += safeParseDurationSeconds(leg);
            tr.duration_seconds = sumSec;
        }

        routes.push_back(tr);
    }

    return routes;
}



/*
std::vector<TrainRoute> findTransitRoutes(const std::string& origin, const std::string& destination) {
    return findDirections(origin, destination, "transit");
}

// Generate Google Maps directions URL
string getDirectionsUrl(const string& origin, const string& destination, const string& mode) {
    string encodedOrigin = urlEncode(origin);
    string encodedDestination = urlEncode(destination);
    
    string modeParam;
    if (mode == "transit") modeParam = "3";
    else if (mode == "driving") modeParam = "0";
    else if (mode == "walking") modeParam = "2";
    else modeParam = "3"; // default to transit
    
    return "https://www.google.com/maps/dir/" + encodedOrigin + "/" + encodedDestination + 
           "/data=!3m1!4b1!4m2!4m1!3e" + modeParam;
}

// Overloaded function for default transit mode
string getDirectionsUrl(const string& origin, const string& destination) {
    return getDirectionsUrl(origin, destination, "transit");
}
// ADD THESE HELPER FUNCTIONS TO maps_api.cpp (at the end of the file)

// Helper function to extract minutes from duration string
int extractMinutes(const string& duration) {
    // Convert "5 hours 30 mins" to minutes
    int hours = 0, minutes = 0;
    size_t hourPos = duration.find("hour");
    size_t minPos = duration.find("min");
    
    if (hourPos != string::npos) {
        hours = stoi(duration.substr(0, hourPos));
    }
    if (minPos != string::npos) {
        size_t spacePos = duration.rfind(' ', minPos);
        if (spacePos != string::npos) {
            minutes = stoi(duration.substr(spacePos + 1, minPos - spacePos - 1));
        }
    }
    
    return hours * 60 + minutes;
}

// Helper function to calculate transport cost
double calculateTransportCost(const string& distanceStr, const string& mode) {
    // Extract numeric distance from string like "350 km"
    double distance = 0.0;
    size_t kmPos = distanceStr.find("km");
    if (kmPos != string::npos) {
        try {
            distance = stod(distanceStr.substr(0, kmPos));
        } catch (...) {
            distance = 100.0; // Default fallback
        }
    } else {
        distance = 100.0; // Default fallback
    }
    
    if (mode == "AUTO") return distance * 12.0;
    if (mode == "BUS") return distance * 1.5;
    if (mode == "TRAIN") return distance * 2.0;
    return distance * 10.0;
}

// Helper function to find optimal hub station
string findOptimalHub(const string& source, const string& destination) {
    // Dynamic hub selection based on geography
    // Common major hubs in India
    vector<string> majorHubs = {
        "MUMBAI CENTRAL", "DELHI JUNCTION", "KOLKATA HOWRAH", 
        "CHENNAI CENTRAL", "BANGALORE CITY", "HYDERABAD DECCAN",
        "AHMEDABAD JUNCTION", "PUNE JUNCTION", "JAIPUR JUNCTION",
        "LUCKNOW JUNCTION", "PATNA JUNCTION", "BHUBANESWAR",
        "VIJAYAWADA JUNCTION", "SURAT", "INDORE JUNCTION"
    };
    
    // For now, return a common hub - in real implementation, 
    // use Dijkstra's algorithm to find optimal connection point
    return "VIJAYAWADA JUNCTION";
}
*/
// Helper function to extract numeric distance from string
double extractDistance(const string& distanceStr) {
    try {
        size_t kmPos = distanceStr.find("km");
        if (kmPos != string::npos) {
            // Extract number before "km"
            string numStr = distanceStr.substr(0, kmPos);
            // Remove any spaces
            numStr.erase(remove(numStr.begin(), numStr.end(), ' '), numStr.end());
            return stod(numStr);
        }
        
        size_t mPos = distanceStr.find("m");
        if (mPos != string::npos && distanceStr.find("min") == string::npos) {
            // Extract number before "m" (meters, not minutes)
            string numStr = distanceStr.substr(0, mPos);
            numStr.erase(remove(numStr.begin(), numStr.end(), ' '), numStr.end());
            return stod(numStr) / 1000.0; // Convert meters to km
        }
    } catch (const exception& e) {
        cout << "⚠️  Distance extraction failed, using default: " << e.what() << endl;
    }
    return 100.0; // default fallback in km
}

// =====================================================================
// ✅ Helper: Build a Google Maps Directions URL
// =====================================================================
std::string getDirectionsUrl(const std::string &from,
                             const std::string &to,
                             const std::string &mode)
{
    std::string f = from, t = to, m = mode;
    // Replace spaces with '+' for URL
    std::replace(f.begin(), f.end(), ' ', '+');
    std::replace(t.begin(), t.end(), ' ', '+');
    std::replace(m.begin(), m.end(), ' ', '+');

    // Generate Google Maps Directions URL
    return "https://www.google.com/maps/dir/?api=1&origin=" + f +
           "&destination=" + t + "&travelmode=" + m;
}
// =====================================================================
// ✅ Overload for 2 arguments (defaults to driving)
// =====================================================================
std::string getDirectionsUrl(const std::string &from,
                             const std::string &to)
{
    return getDirectionsUrl(from, to, "driving");
}


// ============================================================
//  GOOGLE GEOCODING API
// ============================================================

static size_t geocodeWriteCallback(void* contents, size_t size, size_t nmemb, string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}




// ============================================================
//  GOOGLE DISTANCE MATRIX API
// ============================================================



static size_t dmCallback(void* contents, size_t size, size_t nmemb, string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

DistanceMatrixResult getDistanceMatrix(const string& origin, const string& dest) {
    DistanceMatrixResult result {0, 0.0, false};
    string key = GOOGLE_MAPS_API_KEY;      // <<-- use your existing macro

    auto encode = [&](string s){
        // minimal encoder for spaces (maps_api has better encoder elsewhere; this is simple)
        std::string out;
        for (char c: s) {
            if (c == ' ') out.push_back('+');
            else out.push_back(c);
        }
        return out;
    };

    string url = "https://maps.googleapis.com/maps/api/distancematrix/json?origins="
                 + encode(origin)
                 + "&destinations=" + encode(dest)
                 + "&mode=driving"
                 + "&key=" + key;

    CURL* curl = curl_easy_init();
    string response;
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        //curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback); // use your existing callback name
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);

        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }

    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(response, root)) return result;

    if (root.isMember("rows") && root["rows"].size() > 0 &&
        root["rows"][0].isMember("elements") && root["rows"][0]["elements"].size() > 0) {
        auto &elem = root["rows"][0]["elements"][0];
        if (elem.isMember("status") && elem["status"].asString() == "OK") {
            if (elem.isMember("duration") && elem["duration"].isMember("value"))
                result.durationSec = elem["duration"]["value"].asInt();
            if (elem.isMember("distance") && elem["distance"].isMember("value"))
                result.distanceKm = elem["distance"]["value"].asDouble() / 1000.0;
            result.ok = true;
        }
    }
    return result;
}
int extractMinutes(const string& durationStr) {
    // Examples: "5h 30m", "3h", "45m", "2 hrs 10 mins"
    int hours = 0, mins = 0;
    std::string s = durationStr;

    // normalize lowercase
    for (char &c: s) c = std::tolower(c);

    // parse hours
    size_t hpos = s.find("h");
    if (hpos != string::npos) {
        // find number before 'h'
        size_t start = s.rfind(' ', hpos);
        if (start == string::npos) start = 0;
        hours = std::stoi(s.substr(start, hpos - start));
    }

    // parse minutes
    size_t mpos = s.find("m");
    if (mpos != string::npos) {
        size_t start = s.rfind(' ', mpos);
        if (start == string::npos) start = 0;
        mins = std::stoi(s.substr(start, mpos - start));
    }

    return hours * 60 + mins;
}

