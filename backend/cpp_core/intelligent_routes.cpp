// intelligent_routes.cpp
// Upgraded Option 2: fixes for totals + explore placement + clearer guidance
#include "intelligent_routes.h"
#include "maps_api.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <unordered_map>
using namespace std;

/* ---------- Helpers ---------- */

static string toUpper(const string &s) {
    string out; out.reserve(s.size());
    for (char c : s) out.push_back(toupper(c));
    return out;
}

static string prettyDuration(unsigned long sec) {
    unsigned long m = sec / 60;
    unsigned long h = m / 60;
    unsigned long mm = m % 60;
    ostringstream os;
    if (h > 0) os << h << " hr ";
    os << mm << " min";
    return os.str();
}

static string modeIcon(const string &mode) {
    string m = toUpper(mode);
    if (m.find("BUS") != string::npos) return "🚌";
    if (m.find("TRAIN") != string::npos) return "🚆";
    if (m.find("WALK") != string::npos) return "🚶";
    if (m.find("DRIV") != string::npos) return "🚗";
    if (m.find("EXPLORE") != string::npos) return "🌄";
    if (m.find("CAB") != string::npos) return "📍";
    return "📍";
}

extern vector<string> getAttractionsAlongRoute(const string&, const string&, int);
extern double extractDistance(const std::string &); // from maps_api
extern unsigned long parseDurationTextToSeconds(const std::string &); // optional helper

static unsigned long safeParseDuration(const std::string &txt, unsigned long fallbackSec) {
    if (txt.empty()) return fallbackSec;
    try {
        return parseDurationTextToSeconds(txt);
    } catch(...) {
        return fallbackSec;
    }
}

bool placeBelongsToCity(const string &place, const string &cityName) {
    string p = toUpper(place);
    string c = toUpper(cityName);
    return !cityName.empty() && p.find(c) != string::npos;
}

/* ---------- Journey UI ---------- */

static void printStep(const JourneyStep &s) {
    cout << "   ┌───────────────────────────────────────────────┐\n";
    cout << "   │ " << modeIcon(s.mode) << " " << s.mode;
    if (!s.vehicle.empty()) cout << " • " << s.vehicle;
    cout << "\n";

    if (!s.from.empty() || !s.to.empty()) {
        cout << "   │ From: " << s.from << "  →  To: " << s.to << "\n";
    }

    if (!s.departTime.empty() || !s.arriveTime.empty()) {
        cout << "   │ 🕒 "
             << (s.departTime.empty() ? "--:--" : s.departTime)
             << " → "
             << (s.arriveTime.empty() ? "--:--" : s.arriveTime) << "\n";
    }

    if (s.durationSec > 0 || s.distanceKm > 0) {
        cout << "   │ ⏱ " << prettyDuration(s.durationSec)
             << "    📏 " << fixed << setprecision(1) << s.distanceKm << " km\n";
    }

    if (!s.instruction.empty()) {
        cout << "   │ 💡 " << s.instruction << "\n";
    }

    cout << "   └───────────────────────────────────────────────┘\n\n";
}
/*
void displayJourneyPlan(const TrainJourney &j, int id) {
    cout << "\n============================================================\n";
    cout << "----- Recommended Route " << id << " -----\n";
    cout << "Total duration (approx): " << prettyDuration(j.totalDuration) << "\n";
    cout << "Total distance (approx): " << fixed << setprecision(1) << j.totalDistance << " km\n";
    cout << "Transfers: " << j.transferCount << "\n\n";
    cout << "   👉 Continue your journey as guided below.\n\n";
    for (auto &s : j.steps) printStep(s);
}
*/
void displayJourneyPlan(const TrainJourney &journey, int routeNo) {

    cout << "ROUTE_START\n";

    cout << "route:" << routeNo << "\n";
    cout << "duration:" << journey.totalDuration << "\n";
    cout << "distance:" << journey.totalDistance << "\n";
    cout << "transfers:" << journey.transferCount << "\n";

    for (const auto &step : journey.steps) {
/*
        cout << "STEP_START\n";

        // ✅ USE YOUR ACTUAL FIELD NAMES
        cout << "type:" << step.mode << "\n";           // ⚠️ check: mode / type
        cout << "from:" << step.from << "\n";
        cout << "to:" << step.to << "\n";
        cout << "departure:" << step.departTime << "\n";

        // duration conversion (VERY IMPORTANT 🔥)
        int hrs = step.durationSec / 3600;
        int mins = (step.durationSec % 3600) / 60;
        cout << "duration:" << hrs << " hr " << mins << " min\n";

        cout << "distance:" << step.distanceKm << "\n";

        cout << "STEP_END\n";
        */
cout << "STEP_START\n";

// 🔥 TYPE (BUS / CAB / EXPLORE)
cout << "type:" << step.mode << "\n";

// 🔥 NAME (VERY IMPORTANT — FIXES YOUR ISSUE)
cout << "name:" << step.vehicle << "\n";

// 🔥 FROM / TO (FIX EMPTY CASE)
//cout << "from:" << (step.from.empty() ? "-" : step.from) << "\n";
cout << "from:" << (step.from.empty() ? "Nearby Location" : step.from) << "\n";
cout << "to:" << (step.to.empty() ? "-" : step.to) << "\n";

// 🔥 TIME (BOTH DEPARTURE + ARRIVAL)
cout << "departure:" << (step.departTime.empty() ? "-" : step.departTime) << "\n";
cout << "arrival:" << (step.arriveTime.empty() ? "-" : step.arriveTime) << "\n";

// 🔥 DURATION (CONVERT SECONDS → HUMAN)
int hrs = step.durationSec / 3600;
int mins = (step.durationSec % 3600) / 60;
cout << "duration:" << hrs << " hr " << mins << " min\n";

// 🔥 DISTANCE
cout << "distance:" << step.distanceKm << "\n";

// 🔥 NOTE (IMPORTANT FOR EXPLORE)
cout << "note:" << step.instruction << "\n";

cout << "STEP_END\n";


    }

    cout << "ROUTE_END\n";
}



/* ---------- small utilities ---------- */

// create a standardized explore step (definition)
static JourneyStep makeExploreStep(const std::string &title,
                                   unsigned long mins,
                                   double distKm,
                                   const std::string &note)
{
    JourneyStep ex;
    ex.mode = "EXPLORE";
    ex.vehicle = "";
    ex.from = "";
    ex.to = title;
    ex.durationSec = mins * 60;
    ex.distanceKm = distKm;
    ex.instruction = note;
    return ex;
}

// recompute totals from steps (accurate)
static void recomputeJourneyTotals(TrainJourney &tj) {
    unsigned long totalSec = 0;
    double totalKm = 0.0;
    int transfers = 0;
    for (const auto &s : tj.steps) {
        totalSec += s.durationSec;
        totalKm += s.distanceKm;
        // treat transit steps as transfers (BUS/TRAIN/DRIVE/CAB)
        string su = toUpper(s.mode);
        if (su.find("BUS") != string::npos || su.find("TRAIN") != string::npos ||
            su.find("DRIVE") != string::npos || su.find("CAB") != string::npos ||
            su.find("TRANSIT") != string::npos) transfers++;
    }
    tj.totalDuration = totalSec;
    tj.totalDistance = totalKm;
    tj.transferCount = transfers > 0 ? transfers - 1 : 0; // transfers = segments - 1
}

/* ---------- Convert Google route -> JourneyStep(s) ---------- */

static vector<JourneyStep> convertGoogleRouteToSteps(const TrainRoute &r) {
    vector<JourneyStep> out;
    if (!r.trains.empty()) {
        for (const auto &t : r.trains) {
            JourneyStep s;
            string vt = t.vehicleType;
            string vtU = toUpper(vt);
            if (vtU.find("BUS") != string::npos) s.mode = "BUS";
            else if (vtU.find("TRAIN") != string::npos || vtU.find("RAIL") != string::npos) s.mode = "TRAIN";
            else if (vtU.find("DRIVE") != string::npos) s.mode = "DRIVE";
            else s.mode = vt.empty() ? "TRANSIT" : vt;

            s.vehicle = t.trainName.empty() ? t.short_name : t.trainName;
            s.from = t.departureStation;
            s.to = t.arrivalStation;
            s.departTime = t.departureTimeText.empty() ? t.departureTime : t.departureTimeText;
            s.arriveTime = t.arrivalTimeText.empty() ? t.arrivalTime : t.arrivalTimeText;

            if (t.departureTimeEpoch && t.arrivalTimeEpoch) {
                s.durationSec = (unsigned long)(t.arrivalTimeEpoch - t.departureTimeEpoch);
            } else {
                s.durationSec = safeParseDuration(t.duration, 0);
            }

            try { s.distanceKm = extractDistance(t.distance); } catch(...) { s.distanceKm = 0.0; }

            if (!t.headsign.empty()) s.instruction = t.headsign;
            else if (!t.agency.empty()) s.instruction = t.agency;
            else if (!s.vehicle.empty()) s.instruction = s.vehicle;

            out.push_back(s);
        }
    } else {
        JourneyStep s;
        s.mode = "DRIVE";
        s.vehicle = "Road";
        s.from = "Start";
        s.to = "End";
        s.durationSec = r.duration_seconds;
        try { s.distanceKm = extractDistance(r.distance); } catch(...) { s.distanceKm = 0.0; }
        s.instruction = "Follow Google Maps directions (driving/walking)";
        out.push_back(s);
    }
    return out;
}

/* ---------- Create exploring variant: insert explore steps aligned by step.to ---------- */

static TrainJourney createExploringVariant(const TrainRoute &baseRoute,
                                           const vector<string> &places,
                                           unsigned long extraPerPlaceSec)
{
    TrainJourney tj;
    tj.steps = convertGoogleRouteToSteps(baseRoute);

    // total route distance (if available)
    double baseDistance = 0.0;
    try { baseDistance = extractDistance(baseRoute.distance); } catch(...) { baseDistance = 0.0; }

    // average small explore distance computed as a fraction of route distance or fallback to sensible values
    double avgExploreDist = 0.5;
    if (baseDistance > 10.0) {
        avgExploreDist = std::min(5.0, baseDistance / 20.0); // not huge, used as fallback
    }

    // walk through requested places and insert explore steps near the most suitable existing step
    for (const string &place : places) {
        bool inserted = false;
        // Try to find a step whose 'to'/city contains the place (or vice-versa) and insert immediately after
        for (size_t i = 0; i < tj.steps.size(); ++i) {
            const string &city = tj.steps[i].to;
            if (!city.empty() && (placeBelongsToCity(place, city) || placeBelongsToCity(city, place))) {
                // create a helpful instruction including where it was found
                string note = "Found near " + city + ". Suggested time: " + prettyDuration(extraPerPlaceSec)
                              + ". After exploring, return to this pickup point and continue your journey.";
                // use a pragmatic distance (attempt to use baseDistance fraction + clamp)
                double d = avgExploreDist;
                JourneyStep ex = makeExploreStep(place, extraPerPlaceSec/60, d, note);
                // put right after this step so the user sees it inline
                tj.steps.insert(tj.steps.begin() + i + 1, ex);
                inserted = true;
                break;
            }
        }
        if (!inserted) {
            // push to end but still provide instruction referencing last known stop
            string near = tj.steps.empty() ? "near route" : tj.steps.back().to;
            string note = "Found near " + near + ". Suggested time: " + prettyDuration(extraPerPlaceSec)
                          + ". Short detour — then rejoin main route at the same stop.";
            double d = avgExploreDist;
            tj.steps.push_back(makeExploreStep(place, extraPerPlaceSec/60, d, note));
        }
    }

    // recompute totals based on step values (accurate)
    recomputeJourneyTotals(tj);

    return tj;
}

/* ---------- Helper to create an alternate route if Google returns few alternatives ---------- */

static TrainRoute makeAlternateRouteFromBase(const TrainRoute &base) {
    TrainRoute alt = base;
    if (alt.trains.size() >= 2) {
        auto t0 = alt.trains[0];
        alt.trains[0] = alt.trains[1];
        alt.trains[1] = t0;
    } else {
        alt.duration_seconds = base.duration_seconds + 15*60; // +15 mins
        // If base has distance try nudging it a bit
        try {
            // if base.distance parseable, add small extra (maps_api::extractDistance used elsewhere)
        } catch(...) {}
    }
    return alt;
}

/* ========== MAIN: findIntelligentRoutes ========== */

std::vector<TrainJourney> findIntelligentRoutes(const std::string &origin, const std::string &destination) {
    vector<TrainJourney> out;

    vector<TrainRoute> gm = findDirections(origin, destination, "transit");
    if (gm.empty()) gm = findDirections(origin, destination, "driving");
    if (gm.empty()) gm = findDirections(origin, destination, "walking");

    if (gm.empty()) {
        TrainJourney tj;
        JourneyStep s;
        s.mode = "LOCAL";
        s.instruction = "No routes found from Google Maps. Try using city names or check your network/API key.";
        tj.steps.push_back(s);
        recomputeJourneyTotals(tj);
        out.push_back(tj);
        return out;
    }

    TrainRoute r1 = gm[0];
    TrainRoute r2, r3;
    if (gm.size() >= 3) {
        r2 = gm[1];
        r3 = gm[2];
    } else if (gm.size() == 2) {
        r2 = gm[1];
        r3 = makeAlternateRouteFromBase(gm[0]);
    } else {
        r2 = makeAlternateRouteFromBase(gm[0]);
        r3 = makeAlternateRouteFromBase(r2);
    }

    // ---------- Route 1: FASTEST (pure r1) ----------
    {
        TrainJourney tj;
        tj.steps = convertGoogleRouteToSteps(r1);

        JourneyStep lastMile;
        lastMile.mode = "CAB";
        lastMile.vehicle = "Auto / Rapido / Taxi";
        lastMile.from = tj.steps.empty() ? "Last Stop" : tj.steps.back().to;
        lastMile.to = destination;
        lastMile.durationSec = 20 * 60;
        lastMile.distanceKm = 7.0;
        lastMile.instruction = string("Auto/rapido usually available. Give the location name clearly: '") + destination + "'.";
        tj.steps.push_back(lastMile);

        recomputeJourneyTotals(tj);
        // ensure the comment on first step
        if (!tj.steps.empty() && !tj.steps[0].instruction.empty()) {
            tj.steps[0].instruction += " | Recommended: Fastest route to reach your destination.";
        }
        out.push_back(tj);
    }

    // ---------- Route 2: ALTERNATE + MINI EXPLORING (2 places) ----------
    {
        vector<string> explore2 = getAttractionsAlongRoute(origin, destination, 2);
        if (explore2.empty()) explore2 = {"Local market / viewpoint", "Popular street food spot"};
        TrainJourney tj = createExploringVariant(r2, explore2, 60*60); // 60 minutes/stop
        // Add a short continuation hint for transit steps
        for (auto &s : tj.steps) {
            if (toUpper(s.mode).find("EXPLORE") == string::npos) {
                if (!s.instruction.empty()) s.instruction += " | After this leg, follow local signs to the next pickup point.";
            } else {
                // explore steps already include "Found near ..."
            }
        }
        recomputeJourneyTotals(tj);
        out.push_back(tj);
    }

    // ---------- Route 3: FULL EXPLORING (up to 5 places) ----------
    {
        vector<string> explore5 = getAttractionsAlongRoute(origin, destination, 5);
        if (explore5.empty()) {
            explore5 = {"Scenic viewpoint", "Historic temple/fort", "Local handicraft market", "Popular cafe", "Sunset point"};
        }
        TrainJourney tj = createExploringVariant(r3, explore5, 90*60); // 90 minutes per stop
        for (auto &s : tj.steps) {
            if (toUpper(s.mode).find("EXPLORE") == string::npos) {
                if (!s.instruction.empty()) s.instruction += " | After arriving, move to the next pickup location shown below. If unsure, ask locals.";
            }
        }
        recomputeJourneyTotals(tj);
        out.push_back(tj);
    }

    // Make sure the fastest route is first: sort by totalDuration ascending.
    sort(out.begin(), out.end(), [](const TrainJourney &a, const TrainJourney &b){
        return a.totalDuration < b.totalDuration;
    });

    return out;
}
