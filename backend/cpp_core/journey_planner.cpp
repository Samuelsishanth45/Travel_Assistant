// journey_planner.cpp
// Patched: polished display (emojis, no ANSI colors) + direct-option guidance + AI-Mix step limits + duration fix
// Compatible with existing headers and project structure.

#include "journey_planner.h"
#include "train.h"
#include "direct_trains.h"
#include "station_utils.h"
#include "maps_api.h"
#include "intelligent_routes.h"
#include "csv_parser.h"
#include "station_mapper.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <limits>
#include <climits>
#include <cstdio>
#include "train_matrix.h"


using namespace std;

namespace journey_planner {

// Toggle debug
static bool VERBOSE = false;
static inline void dbg(const string &m) { if (VERBOSE) cerr << "[JPL] " << m << endl; }

// -------------------- helpers --------------------
static string nowHHMM() {
    time_t t = time(nullptr);
    tm *lt = localtime(&t);
    if (!lt) return "08:00";
    char buf[9];
    strftime(buf, sizeof(buf), "%H:%M:%S", lt);
    return string(buf);
}
static string addMinutes(const string &hhmmss, long m) {
    int hh=0, mm=0, ss=0;
    if (sscanf(hhmmss.c_str(), "%d:%d:%d", &hh, &mm, &ss) < 2) { hh = 8; mm = 0; ss = 0; }
    long total = hh*3600 + mm*60 + ss + m*60;
    total %= (24*3600);
    if (total < 0) total += 24*3600;
    int nh = (int)(total/3600), nm = (int)((total%3600)/60), ns = (int)(total%60);
    char buf[9]; snprintf(buf, sizeof(buf), "%02d:%02d:%02d", nh, nm, ns);
    return string(buf);
}
static string hhmmFromHHMMSS(const string &t) {
    // convert "HH:MM:SS" or "HH:MM" to "HH:MM"
    int hh=0, mm=0, ss=0;
    if (sscanf(t.c_str(), "%d:%d:%d", &hh, &mm, &ss) >= 2) {
        char buf[6]; snprintf(buf, sizeof(buf), "%02d:%02d", hh, mm);
        return string(buf);
    }
    return t;
}
long parseTimeHHMMSS(const string &t) {
    int hh=0, mm=0, ss=0;
    if (sscanf(t.c_str(), "%d:%d:%d", &hh, &mm, &ss) < 2) {
        // try HH:MM
        if (sscanf(t.c_str(), "%d:%d", &hh, &mm) < 2) return 0;
        ss = 0;
    }
    return hh*3600 + mm*60 + ss;
}

static unsigned long computeDurationFromTimes(const string &depart, const string &arrive) {
    // Parse either "HH:MM:SS" or "HH:MM" formats. Return seconds, handles next-day wrap.
    int dh=0, dm=0, ds=0, ah=0, am=0, asv=0;
    if (sscanf(depart.c_str(), "%d:%d:%d", &dh, &dm, &ds) < 2) {
        dh = dm = ds = 0;
    }
    if (sscanf(arrive.c_str(), "%d:%d:%d", &ah, &am, &asv) < 2) {
        ah = am = asv = 0;
    }
    long dsec = dh*3600 + dm*60 + ds;
    long asec = ah*3600 + am*60 + asv;
    long diff = asec - dsec;
    if (diff < 0) diff += 24*3600; // next day
    return (unsigned long)diff;
}
static string fmtDuration(unsigned long sec) {
    unsigned long m = sec/60;
    unsigned long h = m/60;
    m %= 60;
    ostringstream os;
    if (h>0) os << h << "h " << m << "m";
    else os << m << "m";
    return os.str();
}
static string normalize(const string &s) {
    string out;
    for (char c: s) {
        if (isalnum((unsigned char)c) || isspace((unsigned char)c) || c=='-' || c==',' || c=='.') out.push_back((char)tolower(c));
    }
    size_t a = out.find_first_not_of(' ');
    if (a==string::npos) return "";
    size_t b = out.find_last_not_of(' ');
    return out.substr(a, b-a+1);
}

// -------------------- distance matrix helpers --------------------
DistanceMatrixResult guardedDistanceMatrix(const string &a, const string &b) {
    DistanceMatrixResult r; r.ok = false;
    if (a.empty() || b.empty()) return r;
    string na = normalize(a), nb = normalize(b);
    if (na.size() < 2 || nb.size() < 2) return r;
    if (na == "india" || nb == "india") return r;
    try {
        r = getDistanceMatrix(a,b);
    } catch (...) {
        r.ok = false;
    }
    return r;
}
DistanceMatrixResult estimateFallback(double kmApprox) {
    DistanceMatrixResult r; r.ok = true;
    r.distanceKm = kmApprox>0.0 ? kmApprox : 50.0;
    r.durationSec = (unsigned long)round((r.distanceKm/60.0)*3600.0);
    return r;
}

// -------------------- build step helpers --------------------
JourneyStep buildStep(const string &mode,
                             const string &vehicle,
                             const string &from,
                             const string &to,
                             unsigned long durationSec,
                             double distanceKm,
                             const string &departTime,
                             const string &arriveTime,
                             const string &instruction = "",
                             const string &mapsUrl = "")
{
    JourneyStep s;
    s.mode = mode;
    s.vehicle = vehicle;
    s.from = from;
    s.to = to;
    s.durationSec = durationSec;
    s.distanceKm = distanceKm;
    s.departTime = departTime;
    s.arriveTime = arriveTime;
    s.instruction = instruction;
    s.mapsUrl = mapsUrl;
    // If depart/arrive present but durationSec looks wrong or zero, recompute
    if ((!s.departTime.empty() && !s.arriveTime.empty()) && (s.durationSec == 0 || s.durationSec < 60)) {
        unsigned long d = computeDurationFromTimes(s.departTime, s.arriveTime);
        if (d > 0) s.durationSec = d;
    }
    return s;
}

//JourneyStep buildLocalStep(const string &from, const string &to, unsigned long durSec, double km, const string &note = "") 
JourneyStep buildLocalStep(const string &from, const string &to, unsigned long durSec, double km, const string &note)
{
    string instr = note.empty() ? ("Auto/Walk: " + from + " → " + to) : note;
    return buildStep("LOCAL","Auto/Walk", from, to, durSec, km, "", "", instr, "");
}

// -------------------- sanitize POI --------------------
static bool isSuspiciousPOI(const string &name) {
    string n = normalize(name);
    if (n.find("pakistan")!=string::npos) return true;
    if (n.find("flag")!=string::npos && (n.find("national")!=string::npos || n.find("pakistan")!=string::npos)) return true;
    if (n.find("flag")!=string::npos && n.size()<20) return true;
    return false;
}

static JourneyStep sanitizeAndConvertStep(const JourneyStep &st, const string &originHint = "", const string &destHint = "") {
    if (!st.from.empty() && isSuspiciousPOI(st.from)) { JourneyStep e; e.mode=""; return e; }
    if (!st.to.empty() && isSuspiciousPOI(st.to)) { JourneyStep e; e.mode=""; return e; }

    // If it's train keep but ensure duration computed correctly
    if (st.mode == "TRAIN") {
        JourneyStep out = st;
        if ((out.durationSec == 0) && (!out.departTime.empty() && !out.arriveTime.empty())) {
            out.durationSec = computeDurationFromTimes(out.departTime, out.arriveTime);
        }
        if (out.distanceKm == 0.0) {
            DistanceMatrixResult dm = guardedDistanceMatrix(out.from.empty()?originHint:out.from, out.to.empty()?destHint:out.to);
            if (dm.ok) out.distanceKm = dm.distanceKm;
        }
        if (out.vehicle.empty()) out.vehicle = "Train Service";
        return out;
    }

    // else classify as BUS
    string from = st.from.empty() ? originHint : st.from;
    string to   = st.to.empty()   ? destHint   : st.to;
    string busLabel;
    if (!st.vehicle.empty()) busLabel = st.vehicle;
    //else if (!st.instruction.empty()) busLabel = "Bus: " + trim; // fallback label (trim not defined in header); we'll do simple
    else if (!st.instruction.empty()) busLabel = "Bus: " + st.instruction;

    if (busLabel.empty()) busLabel = "Bus Service: " + (from.empty()? "stop" : from) + " → " + (to.empty()? "stop" : to);

    unsigned long dur = st.durationSec;
    double km = st.distanceKm;
    if (dur == 0 || km == 0.0) {
        DistanceMatrixResult dm = guardedDistanceMatrix(from, to);
        if (dm.ok) { if (dur==0) dur = dm.durationSec; if (km==0.0) km = dm.distanceKm; }
        else { if (dur==0) dur = 30*60; if (km==0.0) km = 20.0; }
    }
    string instr = st.instruction.empty() ? ("Bus: " + from + " → " + to) : st.instruction;
    JourneyStep out = buildStep("BUS", busLabel, from, to, dur, km, st.departTime, st.arriveTime, instr, st.mapsUrl);
    return out;
}



// -------------------- Direct-first plan --------------------
//std::vector<JourneyStep> buildDirectFirstPlan(const string &origin, const string &dest, const std::vector<Train> &allTrains) 
std::vector<JourneyStep> buildDirectFirstPlan(const string &origin,const string &dest){
    vector<JourneyStep> result;
    dbg("buildDirectFirstPlan: " + origin + " -> " + dest);

    string srcCode = findStationCode(origin);
string dstCode = findStationCode(dest);

// fallback if not found
if (srcCode.empty()) srcCode = origin;
if (dstCode.empty()) dstCode = dest;
   // vector<Train> direct = findDirectTrains(origin, dest);
vector<Train> direct = findDirectTrains(srcCode, dstCode);
    if (!direct.empty()) {
        const Train &tr = direct.front();
        // first-mile
        DistanceMatrixResult fm = guardedDistanceMatrix(origin, tr.source);
        if (!fm.ok) fm = estimateFallback(min(10.0, tr.distanceKm>0?tr.distanceKm*0.03:5.0));
        if (fm.ok && fm.durationSec>0) result.push_back(buildLocalStep(origin, tr.source, fm.durationSec, fm.distanceKm, "First-mile to station"));

        // train leg: compute duration properly using train fields or depart/arrive
        unsigned long trainSec = 0;
        if (tr.duration > 0) trainSec = (unsigned long)round(tr.duration * 60.0);
        string depart = tr.departure, arrive = tr.arrival;
        if (trainSec == 0 && !depart.empty() && !arrive.empty()) trainSec = computeDurationFromTimes(depart, arrive);
        if (trainSec == 0) {
            DistanceMatrixResult dd = guardedDistanceMatrix(tr.source, tr.destination);
            if (dd.ok) trainSec = dd.durationSec;
            else trainSec = (unsigned long)round((tr.distanceKm>0?tr.distanceKm:100.0)/60.0*3600.0);
        }
        double trainKm = (tr.distanceKm>0.0 ? tr.distanceKm : tr.distance);
        string veh = (tr.trainNumber.empty() ? tr.trainName : (tr.trainNumber + " - " + tr.trainName));
        result.push_back(buildStep("TRAIN", veh, tr.source, tr.destination, trainSec, trainKm, depart, arrive, string("Board ") + veh));

        // last-mile
        DistanceMatrixResult lm = guardedDistanceMatrix(tr.destination, dest);
        if (!lm.ok) lm = estimateFallback(min(10.0, trainKm*0.03));
        if (lm.ok && lm.durationSec>0) result.push_back(buildLocalStep(tr.destination, dest, lm.durationSec, lm.distanceKm, "Last-mile to destination"));

        return result;
    }

    // 2) try find single direct bus via maps (transit)
    vector<TrainRoute> transitOptions;
    try {
        transitOptions = findTransitRoutes(origin, dest);
    } catch (...) { transitOptions.clear(); }
    // pick option with minimal transfers and ideally single-leg bus
    int bestIdx = -1; unsigned long bestTransfers = ULLONG_MAX;
    for (size_t i=0;i<transitOptions.size();++i) {
       // unsigned long transfers = transitOptions[i].legs.size()>0 ? (unsigned long)transitOptions[i].legs.size()-1 : 0;
        unsigned long transfers =
    transitOptions[i].trains.size() > 0 ?
    (unsigned long)transitOptions[i].trains.size() - 1 :
    0;

        if (transfers < bestTransfers) { bestTransfers = transfers; bestIdx = (int)i; }
    }
    if (bestIdx != -1 && bestTransfers <= 1) {
        // build compact auto->bus->auto
        auto &opt = transitOptions[bestIdx];
        // first-mile
        string busFrom = origin;
        string busTo = dest;
        DistanceMatrixResult fm = guardedDistanceMatrix(origin, busFrom);
        if (!fm.ok) fm = estimateFallback(2.0);
        if (fm.ok && fm.durationSec>0) result.push_back(buildLocalStep(origin, busFrom, fm.durationSec, fm.distanceKm, "First-mile to bus boarding"));

        // main bus (condense if needed)
        unsigned long busSec = 0; double busKm = 0.0;
        string busName = "Intercity Bus";
        if (!opt.duration.empty()) {
            // try to parse duration like "PT12H30M" or textual — fallback to estimate
        }
        DistanceMatrixResult md = guardedDistanceMatrix(busFrom, busTo);
        if (md.ok) { busSec = md.durationSec; busKm = md.distanceKm; }
        else { busSec = 10*3600; busKm = 500.0; }
       // result.push_back(buildStep("BUS", opt.name.empty()? "Intercity Bus" : opt.name, busFrom, busTo, busSec, busKm, "", "", "Intercity bus (direct)"));
        result.push_back(buildStep("BUS", busName, busFrom, busTo, busSec, busKm, "", "", "Intercity bus (direct)"));


        // final-mile
        DistanceMatrixResult lm2 = guardedDistanceMatrix(busTo, dest);
        if (!lm2.ok) lm2 = estimateFallback(2.0);
        if (lm2.ok && lm2.durationSec>0) result.push_back(buildLocalStep(busTo, dest, lm2.durationSec, lm2.distanceKm, "Last-mile"));

        return result;
    }

    // 3) no true direct: return a best-effort maps drive leg (but direct option will be considered 'not direct' by display)
    DistanceMatrixResult dm = guardedDistanceMatrix(origin, dest);
    if (dm.ok) {
        result.push_back(buildStep("DRIVE", "Transit/Taxi", origin, dest, dm.durationSec, dm.distanceKm, nowHHMM(), addMinutes(nowHHMM(), (long)round(dm.durationSec/60.0)), "Maps transit/drive"));
    } else {
        DistanceMatrixResult est = estimateFallback(200.0);
        result.push_back(buildStep("DRIVE","Taxi", origin, dest, est.durationSec, est.distanceKm, nowHHMM(), addMinutes(nowHHMM(), (long)round(est.durationSec/60.0)), "Estimated drive (fallback)"));
    }
    return result;
}




// -------------------- Mixed-best plan (limit steps) --------------------
//std::vector<JourneyStep> buildMixedBestPlan(const string &origin, const string &dest, const std::vector<Train> &allTrains)
std::vector<JourneyStep> buildMixedBestPlan(const string &origin,const string &dest) {
    vector<JourneyStep> bestPlan;
    dbg("buildMixedBestPlan: " + origin + " -> " + dest);

    // try nearest stations
    string srcStation = findNearestStation(origin);
    string dstStation = findNearestStation(dest);
    if (srcStation.empty()) srcStation = origin;
    if (dstStation.empty()) dstStation = dest;

    // 1) try direct train mixing if possible
    //vector<Train> direct = findDirectTrains(allTrains, srcStation, dstStation);
   // vector<Train> direct = findDirectTrains(srcStation, dstStation);
    string srcCode = findStationCode(srcStation);
string dstCode = findStationCode(dstStation);

if (srcCode.empty()) srcCode = srcStation;
if (dstCode.empty()) dstCode = dstStation;

vector<Train> direct = findDirectTrains(srcCode, dstCode);

    if (!direct.empty()) {
        const Train &tr = direct.front();
        // first-mile
        DistanceMatrixResult fm = guardedDistanceMatrix(origin, tr.source);
        if (!fm.ok) fm = estimateFallback(3.0);




        if (fm.ok && fm.durationSec>0) bestPlan.push_back(buildLocalStep(origin, tr.source, fm.durationSec, fm.distanceKm, "First-mile to station"));
        // train leg
        unsigned long tsec = 0; if (tr.duration>0) tsec = (unsigned long)round(tr.duration*60.0);
        if (!tr.departure.empty() && !tr.arrival.empty()) {
            unsigned long trainSec = 0;

 long dep = parseTimeHHMMSS(tr.departure);
long arr = parseTimeHHMMSS(tr.arrival);

   // long dep = parseTime(tr.departure);
    //long arr = parseTime(tr.arrival);
    if (arr < dep) arr += 24*3600; // next day
    trainSec = arr - dep;
}

        if (tsec==0 && !tr.departure.empty() && !tr.arrival.empty()) tsec = computeDurationFromTimes(tr.departure, tr.arrival);
        if (tsec==0) { DistanceMatrixResult dt = guardedDistanceMatrix(tr.source, tr.destination); if (dt.ok) tsec = dt.durationSec; else tsec = 3600; }
        string veh = (tr.trainNumber.empty()?tr.trainName:(tr.trainNumber + " - " + tr.trainName));
        bestPlan.push_back(buildStep("TRAIN", veh, tr.source, tr.destination, tsec, (tr.distanceKm>0?tr.distanceKm:tr.distance), tr.departure, tr.arrival, "Board " + veh));
        // last-mile
        DistanceMatrixResult lm = guardedDistanceMatrix(tr.destination, dest);
        if (!lm.ok) lm = estimateFallback(3.0);
        if (lm.ok && lm.durationSec>0) bestPlan.push_back(buildLocalStep(tr.destination, dest, lm.durationSec, lm.distanceKm, "Last-mile to destination"));

        // ensure <=5 steps
       // NEVER compress train+bus into fake leg
if (bestPlan.size() > 5) {
    // Keep first 5 steps maximum
    vector<JourneyStep> limited;
    for (int i = 0; i < 5 && i < bestPlan.size(); i++) {
        limited.push_back(bestPlan[i]);
    }
    return limited;
}

        return bestPlan;
    }

    // 2) try intelligent chain (prefer mix train+bus)
    vector<TrainJourney> chains = findIntelligentRoutes(srcStation, dstStation);
    if (!chains.empty()) {
        // pick best by totalDuration
        size_t bestIdx = 0; unsigned long bestDur = ULLONG_MAX;
        for (size_t i=0;i<chains.size();++i) {
            unsigned long d = chains[i].totalDuration ? chains[i].totalDuration : (unsigned long)round((chains[i].totalDistance/60.0)*3600.0);
            if (d < bestDur) { bestDur = d; bestIdx = i; }
        }
        const TrainJourney &tj = chains[bestIdx];
        // first-mile to first hub
        if (!tj.steps.empty()) {
            JourneyStep first = tj.steps.front();
         //   DistanceMatrixResult fm = guardedDistanceMatrix(origin, first.from);
           // if (!fm.ok) fm = estimateFallback(2.0);

DistanceMatrixResult fm = guardedDistanceMatrix(origin, first.from);

// 🔥 FIX: prevent crazy values
if (!fm.ok || fm.distanceKm > 50 || fm.durationSec > 7200) {
    fm = estimateFallback(3.0); // 3 km, small local move
}


            if (fm.ok && fm.durationSec>0) bestPlan.push_back(buildLocalStep(origin, first.from, fm.durationSec, fm.distanceKm, "First-mile to hub"));
            // append sanitized steps but limit total
            for (const JourneyStep &st : tj.steps) {
                JourneyStep s = sanitizeAndConvertStep(st, first.from, tj.steps.back().to);
                if (!s.mode.empty()) bestPlan.push_back(s);
                if (bestPlan.size() >= 5) break;
            }
            // last-mile
            JourneyStep last = tj.steps.back();
       //     DistanceMatrixResult lm = guardedDistanceMatrix(last.to, dest);
         //   if (!lm.ok) lm = estimateFallback(2.0);
DistanceMatrixResult lm = guardedDistanceMatrix(last.to, dest);

// 🔥 FIX
if (!lm.ok || lm.distanceKm > 50 || lm.durationSec > 7200) {
    lm = estimateFallback(3.0);
}



            if (lm.ok && lm.durationSec>0) bestPlan.push_back(buildLocalStep(last.to, dest, lm.durationSec, lm.distanceKm, "Last-mile to destination"));
        }
        // If steps > 5 compress into 3-5 steps
// NEVER compress into fake mixed leg.
// Limit to first 5 steps maximum to keep output clean.
if (bestPlan.size() > 5) {
    vector<JourneyStep> limited;
    for (int i = 0; i < 5 && i < bestPlan.size(); i++) {
        limited.push_back(bestPlan[i]);
    }
    return limited;
}




        return bestPlan;
    }

    // 3) maps fallback but return compact 3-step bus fallback if possible
    DistanceMatrixResult dm = guardedDistanceMatrix(origin, dest);
    if (!dm.ok || dm.distanceKm > 1500) {
    dm = estimateFallback(400.0); // long travel fallback
}
    if (dm.ok) {
        vector<JourneyStep> out;
        // first-mile (auto)
        DistanceMatrixResult fm = guardedDistanceMatrix(origin, srcStation);
        if (!fm.ok) fm = estimateFallback(2.0);
        if (fm.ok && fm.durationSec>0) out.push_back(buildLocalStep(origin, srcStation, fm.durationSec, fm.distanceKm, "First-mile to bus hub"));
        // main bus (condense)
        out.push_back(buildStep("BUS","Intercity Bus", srcStation, dstStation, dm.durationSec, dm.distanceKm, nowHHMM(), addMinutes(nowHHMM(), (long)round(dm.durationSec/60.0)), "Intercity bus (maps)"));
        // last-mile
        DistanceMatrixResult lm = guardedDistanceMatrix(dstStation, dest);
        if (!lm.ok) lm = estimateFallback(2.0);
        if (lm.ok && lm.durationSec>0) out.push_back(buildLocalStep(dstStation, dest, lm.durationSec, lm.distanceKm, "Last-mile"));
        // Ensure 3-step minimal fallback: if more, compress
        if (out.size() > 3) {
            vector<JourneyStep> comp;
            comp.push_back(out.front());
            comp.push_back(out[1]);
            comp.push_back(out.back());
            return comp;
        }
        return out;
    }

    // last emergency fallback
    DistanceMatrixResult est = estimateFallback(400.0);
    vector<JourneyStep> out;
    out.push_back(buildLocalStep(origin, srcStation, 10*60, 3.0, "First-mile"));
    out.push_back(buildStep("BUS","Long-distance Bus", srcStation, dstStation, est.durationSec, est.distanceKm, "", "", "Long bus fallback"));
    out.push_back(buildLocalStep(dstStation, dest, 10*60, 3.0, "Last-mile"));
    return out;
}

// -------------------- isDirectAvailable checker --------------------
static bool isDirectPlanMeaningful(const vector<JourneyStep> &plan) {
    // return true only if plan contains a TRAIN OR it's a single BUS with <=1 transfer
    if (plan.empty()) return false;
    bool hasTrain = false;
    int busCount = 0;
    for (const JourneyStep &s : plan) {
        if (s.mode == "TRAIN") hasTrain = true;
        if (s.mode == "BUS") busCount++;
    }
    if (hasTrain) return true;
    // if plan is single BUS (one BUS step) and has local steps only around it, treat as meaningful
    if (busCount == 1) return true;
    // otherwise not meaningful as a "Direct"
    return false;
}

// -------------------- DISPLAY helpers (emojis, no colors) --------------------
static string trimEllipsis(const string &s, size_t w) {
    if (s.size() <= w) return s;
    if (w <= 3) return s.substr(0,w);
    return s.substr(0, w-3) + "...";
}
static void printCenteredBoxTitle(const string &title, int width) {
    int w = max(40, width);
    int pad = (w - (int)title.size()) / 2;
    cout << "+" << string(w, '-') << "+" << "\n";
    cout << "|" << string(max(0,pad-1), ' ') << title << string(max(0, w - pad - (int)title.size()), ' ') << "|\n";
    cout << "+" << string(w, '-') << "+" << "\n";
}
static string emojiForMode(const string &mode) {
    if (mode == "TRAIN") return "🚆";
    if (mode == "BUS") return "🚌";
    if (mode == "LOCAL") return "🛺";
    if (mode == "DRIVE") return "🚗";
    return "🌐";
}

static void printStepBox(const JourneyStep &s, int stepNo, int width) {
    int w = max(48, width);
    string emoji = emojiForMode(s.mode);
    string modeLabel = s.mode;
    string vehicle = s.vehicle.empty() ? (s.mode == "LOCAL" ? "Auto/Walk" : (s.mode=="BUS" ? "Bus" : s.mode)) : s.vehicle;
    string timeRange;
    if (!s.departTime.empty() || !s.arriveTime.empty()) {
        string dep = s.departTime.empty() ? "Now" : hhmmFromHHMMSS(s.departTime);
        string arr = s.arriveTime.empty() ? "Later" : hhmmFromHHMMSS(s.arriveTime);
        timeRange = dep + " -> " + arr;
    } else {
        timeRange = fmtDuration(s.durationSec);
    }
    ostringstream head;
    head << "STEP " << stepNo << " : " << emoji << " " << (s.mode == "TRAIN" ? "TRAIN" : (s.mode=="BUS" ? "BUS" : (s.mode=="LOCAL" ? "LOCAL" : (s.mode=="DRIVE"?"DRIVE":"OTHER"))));
    string header = head.str();

    cout << "+" << string(w, '=') << "+\n";
    cout << "| " << left << setw(w-2) << header << " |\n";
    cout << "+" << string(w, '-') << "+\n";
    cout << "| " << left << setw(w-2) << ("Service: " + trimEllipsis(vehicle, (size_t)w-12)) << " |\n";
    string ft = (s.from.empty() ? "-" : s.from) + "  →  " + (s.to.empty() ? "-" : s.to);
    cout << "| " << left << setw(w-2) << ("Route: " + trimEllipsis(ft, (size_t)w-12)) << " |\n";
    string dur = fmtDuration(s.durationSec);
    ostringstream tline; tline << "Time: " << trimEllipsis(timeRange, (size_t)w-16) << "  |  Duration: " << dur;
    cout << "| " << left << setw(w-2) << tline.str() << " |\n";
    if (s.distanceKm > 0.0) {
        ostringstream dline; dline << "Distance: " << fixed << setprecision(1) << s.distanceKm << " km";
        cout << "| " << left << setw(w-2) << dline.str() << " |\n";
    } else {
        cout << "| " << left << setw(w-2) << "Distance: -" << " |\n";
    }
    if (!s.instruction.empty()) cout << "| " << left << setw(w-2) << ("Note: " + trimEllipsis(s.instruction, (size_t)w-12)) << " |\n";
    if (!s.mapsUrl.empty()) cout << "| " << left << setw(w-2) << ("Maps: " + trimEllipsis(s.mapsUrl, (size_t)w-12)) << " |\n";
    cout << "+" << string(w, '=') << "+\n\n";
}

// -------------------- Display main --------------------

// -------------------- Display main --------------------
void displayRealTimeJourney(const std::vector<JourneyStep> &steps, const std::string &origin, const std::string &destination) {
    const int boxWidth = 76;

    // detect route flavor: direct if contains TRAIN and NO LOCAL steps
    bool containsTrain = false;
    bool containsLocal = false;
    for (const JourneyStep &s : steps) {
        if (s.mode == "TRAIN") containsTrain = true;
        if (s.mode == "LOCAL") containsLocal = true;
        // treat DRIVE/BUS as non-local (so mixed)
    }

    // choose header based on detection
    string headerTitle;
    if (containsTrain && !containsLocal) {
        headerTitle = "🚆 DIRECT ROUTE — SIMPLE";
    } else {
        headerTitle = "🤖 AI-MIXED ROUTE — RECOMMENDED";
    }

    cout << "\n";
    printCenteredBoxTitle("COMPLETE JOURNEY — DETAILED VIEW", boxWidth);
    cout << "From: " << origin << "   →   To: " << destination << "\n";
    cout << "Generated: " << nowHHMM() << "\n";
    // print the route kind directly under the generated line for clarity
    cout << left << setw(boxWidth+2) << ("Route type: " + headerTitle) << "\n";
    cout << string(boxWidth+2, '=') << "\n\n";

    if (steps.empty()) {
        cout << "⚠️  No route found.\n";
        cout << string(boxWidth+2, '=') << "\n";
        return;
    }

    // compute total properly (use durations already fixed by buildStep)
    unsigned long totSec = 0; double totKm = 0.0;
    for (const JourneyStep &s : steps) {
        // defensive: if depart/arrive present but duration is tiny, recompute
        totSec += s.durationSec;
        totKm += s.distanceKm;
    }

    // Print summary
    ostringstream sum;
    sum << "SUMMARY: " << (totSec/3600) << "h " << ((totSec%3600)/60) << "m"
        << "  |  " << fixed << setprecision(1) << totKm << " km";
    cout << "+" << string(boxWidth, '-') << "+\n";
    cout << "| " << left << setw(boxWidth-2) << sum.str() << " |\n";
    cout << "+" << string(boxWidth, '-') << "+\n\n";

    // print steps with clear labeling — if this is a direct route we emphasize the TRAIN step
    int idx = 1;
    for (const JourneyStep &s : steps) {
        // For direct route: if step is TRAIN, label it clearly as the direct train
        if (containsTrain && !containsLocal && s.mode == "TRAIN") {
            // emphasize train-only direct route
            JourneyStep tmp = s; // local copy for formatting if needed
            // keep printing using existing helper
            printStepBox(tmp, idx++, boxWidth);
        } else {
            // normal print
            printStepBox(s, idx++, boxWidth);
        }
    }

    cout << string(boxWidth+2, '=') << "\n";
    cout << "✅ End of Plan — Safe travels!\n";
    cout << "Tip: Verify train timings on IRCTC and book buses on RedBus for long legs.\n";
    cout << string(boxWidth+2, '=') << "\n\n";
}



// -------------------- MAIN ROUTE ENGINE (the missing function) --------------------
//std::vector<JourneyStep> getRealTimeJourneyPlan(const std::string& source,const std::string& destination,const std::vector<Train>& allTrains,size_t numCandidateStations)
std::vector<JourneyStep> getRealTimeJourneyPlan(const std::string& source,const std::string& destination,size_t numCandidateStations)

{
    /*
    // 1️⃣ Try Direct-first plan
    std::vector<JourneyStep> directPlan =
        buildDirectFirstPlan(source, destination, allTrains);

    // 2️⃣ Try Mixed-best plan
    std::vector<JourneyStep> mixedPlan =
        buildMixedBestPlan(source, destination, allTrains);
*/
        std::vector<JourneyStep> directPlan =
    buildDirectFirstPlan(source, destination);

std::vector<JourneyStep> mixedPlan =
    buildMixedBestPlan(source, destination);


    // 3️⃣ If direct is meaningful, return direct
bool directMeaningful = false;

// direct is meaningful ONLY IF:
//   1) exactly one train between X → Y   OR
//   2) exactly one bus between X → Y
if (!directPlan.empty()) {
    int trains = 0;
    int buses = 0;

    for (auto &s : directPlan) {
        if (s.mode == "TRAIN") trains++;
        if (s.mode == "BUS") buses++;
    }

    if (trains == 1) directMeaningful = true;
    else if (trains == 0 && buses == 1) directMeaningful = true;
}


    // If direct plan exists and is meaningful → use it
    if (directMeaningful) {
        return directPlan;
    }
    // If direct is NOT meaningful, show message and force AI-Mix
if (!directMeaningful) {
    cout << "❌ No direct train/bus found within 50 km.\n";
    cout << "👉 Please choose AI-Mixed Route (Recommended).\n\n";
}


    // Otherwise fallback to AI-Mix
    return mixedPlan;
}


} // namespace journey_planner

// End of file


namespace journey_planner {

void displayPlannerAPI(const JourneyPlan &plan) {

    cout << "PLAN_START\n";

    cout << "type:AI\n";
    cout << "totalTime:" << plan.totalTime << "\n";
    cout << "distance:" << plan.totalDistance << "\n";

    for (const auto &step : plan.steps) {

        cout << "STEP_START\n";

        cout << "mode:" << step.mode << "\n";
        cout << "service:" << step.service << "\n";
        cout << "from:" << step.from << "\n";
        cout << "to:" << step.to << "\n";
        cout << "time:" << step.time << "\n";
        cout << "duration:" << step.duration << "\n";
        cout << "distance:" << step.distance << "\n";
        cout << "note:" << step.note << "\n";

        cout << "STEP_END\n";
    }

    cout << "PLAN_END\n";
}

} // ✅ IMPORTANT