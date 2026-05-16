#include "tourist.h"
#include "maps_api.h"

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <curl/curl.h>
#include <json/json.h>
#include <iomanip>
#include <algorithm>
#include <map>

using namespace std;

/* ================= STRUCT ================= */
struct Attraction {
    string name;
    string description;
    string best_time;
};

/* ================= CURL ================= */
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

string cleanJSON(string text) {
    size_t start = text.find('[');
    size_t end = text.rfind(']');
    if (start != string::npos && end != string::npos)
        return text.substr(start, end - start + 1);
    return "";
}

/* ================= GOOGLE MAPS CACHE ================= */
map<string, map<string, int>> travelCache;

int getFastTravelTime(string from, string to) {
    if (travelCache[from][to] != 0)
        return travelCache[from][to];

    DistanceMatrixResult res = getDistanceMatrix(from, to);

    if (!res.ok || res.durationSec <= 0) {
        return 60; // safe fallback
    }

    int mins = res.durationSec / 60;
    if (mins <= 0) mins = 30;

    travelCache[from][to] = mins;
    return mins;
}

/* ================= GROQ API ================= */
vector<Attraction> fetchFromGroq(const string& query) {
    vector<Attraction> results;

    string API_KEY = "REMOVED834WzGKI3pUY3QYlQBgbWGdyb3FYTHygszEuyQ9xzwvW7SVfzjQc";

    string url = "https://api.groq.com/openai/v1/chat/completions";

    string prompt =
        "Give 6 to 10 REAL tourist places in " + query +
        ". Return ONLY JSON array like "
        "[{\"name\":\"\",\"description\":\"\",\"best_time\":\"\"}]";

    Json::Value req;
    req["model"] = "llama-3.1-8b-instant";
    req["messages"][0]["role"] = "user";
    req["messages"][0]["content"] = prompt;

    Json::StreamWriterBuilder writer;
    string requestBody = Json::writeString(writer, req);

    for (int attempt = 0; attempt < 2; attempt++) {

        CURL* curl = curl_easy_init();
        if (!curl) return results;

        string response;
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + API_KEY).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 8L);

        CURLcode res = curl_easy_perform(curl);

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);

        if (res != CURLE_OK) continue;

        Json::Value root;
        Json::CharReaderBuilder reader;
        string errs;
        stringstream ss(response);

        if (!Json::parseFromStream(reader, ss, &root, &errs)) continue;

        string content = root["choices"][0]["message"]["content"].asString();
        string jsonArray = cleanJSON(content);

        if (jsonArray.empty()) continue;

        Json::Value arr;
        stringstream ssArr(jsonArray);

        if (!Json::parseFromStream(reader, ssArr, &arr, &errs)) continue;

        for (auto& item : arr) {
            Attraction a;
            a.name = item.get("name", "").asString();
            a.description = item.get("description", "").asString();
            a.best_time = item.get("best_time", "All year").asString();

            if (!a.name.empty())
                results.push_back(a);
        }

        if (!results.empty()) return results;
    }

    return results;
}

/* ================= FALLBACK ================= */
vector<Attraction> fallback(const string& city) {
    return {
        {"City Center", "Explore local highlights", "All year"},
        {"Main Temple", "Famous spiritual place", "Morning"},
        {"Local Market", "Food & shopping", "Evening"},
        {"Lake / Park", "Relaxation spot", "Evening"},
        {"Museum", "Culture & history", "Daytime"}
    };
}

/* ================= OPTION 5 ================= */
void displayTouristAttractions(const string& city) {
    cout << "\n🔎 Exploring attractions in " << city << "...\n";

    auto places = fetchFromGroq(city + ", India");

    if (places.empty()) places = fallback(city);

    cout << "\n====================================================\n";
    cout << "✨ TOP ATTRACTIONS IN " << city << " ✨\n";
    cout << "====================================================\n";

    int i = 1;
    for (auto& p : places) {
        cout << "\n" << setw(2) << i++ << ". 🌟 " << p.name << "\n";
        cout << "     " << p.description << "\n";
        cout << "     🕒 Best time: " << p.best_time << "\n";
    }
}

/* ================= OPTION 4 ================= */     
void planSmartTour(const string& city, int days) {
  
    cout << "\n🧠 Smart AI Travel Planner\n";
    cout << "📍 " << city << " | Days: " << days << "\n";

    auto places = fetchFromGroq(city + " tourist places India");

    if (places.empty()) places = fallback(city);

    // LIMIT places
  //  if (places.size() > 8) places.resize(8);

if (places.size() > days * 3)
    places.resize(days * 3);


    // SCORE by distance
    vector<pair<int, Attraction>> scored;

    for (auto& p : places) {
        string full = p.name + ", " + city + ", India";
        int t = getFastTravelTime(city + ", India", full);

        if (t > 5 && t < 300)
            scored.push_back({t, p});
    }

    sort(scored.begin(), scored.end(),
         [](auto& a, auto& b) { return a.first < b.first; });

    places.clear();
    for (auto& s : scored)
        places.push_back(s.second);

    cout << "\n================ SMART PLAN ================\n";

    /*
    int index = 0;

    for (int d = 1; d <= days; d++) {

        cout << "\n🌅 DAY " << d << " PLAN\n";
        cout << "--------------------------------------------\n";

        int total = 12 * 60; // minutes
        string current = city + ", India";

        cout << "🕗 Start at 8:00 AM\n";

        while (index < places.size() && total > 120) {

            auto& p = places[index++];

            string next = p.name + ", " + city + ", India";

            int travel = getFastTravelTime(current, next);
            int stay = 120; // 2 hrs

            if (travel + stay > total) break;

            cout << "\n➡️ " << p.name << "\n";
            cout << "   🚗 Travel: " << travel << " mins\n";
            cout << "   🏞 Explore: " << stay/60 << " hrs\n";
            cout << "   💡 " << p.description << "\n";

            // Google Maps Link
            cout << "   📍 Route: https://www.google.com/maps/dir/"
                 << current << "/" << next << "\n";

            total -= (travel + stay);
            current = next;

            if (total < 120) break;
        }

        cout << "\n🍽 Try local food in " << city << "\n";
        cout << "🌙 Return to " << city << "\n";
    }
    */
   int totalPlaces = places.size();

if (totalPlaces == 0) {
    cout << "\n❌ No attractions found.\n";
    return;
}

// 🔥 distribute places equally
int placesPerDay = max(1, totalPlaces / days);

int index = 0;

for (int d = 1; d <= days; d++) {

    cout << "\n🌅 DAY " << d << " PLAN\n";
    cout << "--------------------------------------------\n";

    int total = 12 * 60;
    string current = city + ", India";

    cout << "🕗 Start at 8:00 AM\n";

    int coveredToday = 0;

    while (
        index < totalPlaces &&
        coveredToday < placesPerDay &&
        total > 120
    ) {

        auto& p = places[index++];

        string next = p.name + ", " + city + ", India";

        int travel = getFastTravelTime(current, next);
        int stay = 120;

        if (travel + stay > total)
            break;

        cout << "\n➡️ " << p.name << "\n";
        cout << "   🚗 Travel: " << travel << " mins\n";
        cout << "   🏞 Explore: " << stay / 60 << " hrs\n";
        cout << "   💡 " << p.description << "\n";

        cout << "   📍 Route: https://www.google.com/maps/dir/"
             << current << "/" << next << "\n";

        total -= (travel + stay);

        current = next;

        coveredToday++;
    }

    // 🔥 if final day and places remaining
    if (d == days) {

        while (index < totalPlaces) {

            auto& p = places[index++];

            cout << "\n➡️ " << p.name << "\n";
            cout << "   🚗 Travel: 30 mins\n";
            cout << "   🏞 Explore: 2 hrs\n";
            cout << "   💡 " << p.description << "\n";

            cout << "   📍 Route: https://www.google.com/maps/dir/"
                 << current << "/"
                 << p.name << "," << city << "\n";
        }
    }

    cout << "\n🍽 Try local food in " << city << "\n";
    cout << "🌙 Return to " << city << "\n";
}

    cout << "\n===========================================\n";
    cout << "✨ Enjoy your journey ✨\n";
}