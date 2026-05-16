#include "floyd_warshall.h"
#include "indirectroutes.h"
#include "train_matrix.h"
#include "station_utils.h"
#include "station_mapper.h"
#include "train.h"

#include <iostream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <functional>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <limits>
#include <string>
#include <set>
#include <climits>
using namespace std;
 
extern vector<string> stationList;
extern unordered_map<string,int> floydstationIndex;
extern vector<vector<RouteState>> distMatrix;
extern vector<vector<int>> kPaths;
extern vector<int> getshortestpath;

static const int LAYOVER = 120;
const long long INF = 9e18;

struct Journey {
   // vector<Train> trains;
    vector<pair<Train, pair<string,string>>> trains;
    vector<string> stations;  
    long long totalTime;
};


static int toMin(const string &t) {
    return stoi(t.substr(0,2))*60 + stoi(t.substr(3,2));
}

int alignTime(int baseTime, int eventTime)
{
    int t = eventTime;

    while(t < baseTime)
        t += 1440;

    return t;
}

bool getSegmentTimes(const Train &t,const string &u,const string &v,int &dep,int &arr)
{
    int s=-1,e=-1;

    for(int i=0;i<t.stations.size();i++){
        if(t.stations[i]==u) s=i;
        if(t.stations[i]==v){ e=i; break;}
    }

    if(s==-1||e==-1||s>=e) return false;

    //if(e - s != 1) return false;
 dep = toMin(t.departureTimes[s]);
 arr = toMin(t.arrivalTimes[e]);

if(dep < 0 || arr < 0) return false;
/*
int travel = arr - dep;

// basic rollover
if(travel < 0)
    travel += 1440;

// ✅ DISTANCE BASED FIX (YOUR LOGIC)
int dist = t.distances[e] - t.distances[s];

if(dist > 1200 && travel < 1440)
{
    travel += 1440;
}
*/
int travel = arr - dep;

// fix overnight first
if(travel < 0)
    travel += 1440;

// 🔥 ALWAYS check distance (independent condition)
int dist = t.distances[e] - t.distances[s];

if(dist >= 1200)
    travel += 1440;




arr = dep + travel;

if(s == -1 || e == -1 || s >= e)
{
    cout << "DEBUG FAIL: " << u << " -> " << v << endl;
    return false;
}


/*
    dep = toMin(t.departureTimes[s]);
    arr = toMin(t.arrivalTimes[e]);

    if(dep < 0 || arr < 0) return false;

    int duration = arr - dep;

    // overnight
    if(duration < 0)
        duration += 1440;

    // 🔥 FIX: segment distance
    int segmentDistance = 0;

    if(e < t.distances.size())
        segmentDistance = t.distances[e] - t.distances[s];

    // 🔥 multi-day train
    if(segmentDistance >= 1200 && duration < 1440)
        duration += 1440;

    arr = dep + duration;
    */

    return true;
}

string formatTime(int mins)
{
    mins %= 1440;
    if(mins < 0) mins += 1440;

    int h = mins / 60;
    int m = mins % 60;

    char buf[6];
    sprintf(buf, "%02d:%02d", h, m);

    return string(buf);
}

void handleIndirectRoutes(const string &src,const string &dest)
{
    computeTimeFloydMatrix(src,dest);

    if(kPaths.empty()){
        cout<<"❌ No Floyd paths generated\n";
        return;
    }

    int srcIdx = floydstationIndex[src];
    int destIdx = floydstationIndex[dest];

    vector<Journey> allJourneys;

    vector<vector<int>> allPaths = kPaths;

if(allPaths.empty()){
    cout<<"⚠️ No Floyd paths\n";
    return;
}



    for(auto &path : kPaths)
    {
        vector<string> stations;
        for(int i : path)
            stations.push_back(stationList[i]);

          //  if(stations.size() < 2) continue;   ///////////////////////

        unordered_map<string,int> pos;
        for(int i=0;i<stations.size();i++)
            pos[stations[i]] = i;

        struct State {
            int node;
            long long time;
            int arrival;
           // vector<Train> path;
            vector<pair<Train, pair<string,string>>> path;
        };

        struct cmp {
            bool operator()(const State &a,const State &b){
                return a.time > b.time;
            }
        };

        priority_queue<State,vector<State>,cmp> pq;
        pq.push({srcIdx,0,0,{}});

       unordered_map<int,long long> best;

        vector<Journey> journeys;

        while(!pq.empty() && journeys.size()<50)
        {
            auto cur = pq.top(); pq.pop();
            if(cur.time > 5*1440) continue;
          //  if(cur.path.size() > stations.size()) continue;
            if(cur.path.size() > 5) continue;

           if(best.count(cur.node) && best[cur.node] <= cur.time) continue;
            best[cur.node] = cur.time;

/*
            if(cur.node == destIdx){
                journeys.push_back({cur.path, stations, cur.time});
                continue;
            }


if(cur.node == destIdx)
{
    if(cur.path.size() == stations.size()-1)
    {
        journeys.push_back({cur.path, stations, cur.time});
    }
    continue;
}
*/


if(cur.node == destIdx)
{
 long long totalLayover = 0;
long long pureTravel = 0;

for(int j=0; j<cur.path.size(); j++)
{
    int dep, arr;
   // getSegmentTimes(cur.path[j], stations[j], stations[j+1], dep, arr);

    auto &seg = cur.path[j];
//Train t = seg.first;
string u = seg.second.first;
string v = seg.second.second;

const vector<Train> &trains = trainMatrix[u][v];
/*
if(trains.empty())
{
    cout << "⚠️ No train found\n";
    continue;
}
*/

if(trains.empty())
{
     // valid = false;
    break;   
}


// pick BEST train dynamically
Train t = trains[0];
int bestTime = INT_MAX;

for(const Train &cand : trains)
{
    int dep, arr;

    if(!getSegmentTimes(cand, u, v, dep, arr))
        continue;

    int dur = arr - dep;
    if(dur < 0) dur += 1440;

    if(dur < bestTime)
    {
        bestTime = dur;
        t = cand;
    }
}

bool ok = getSegmentTimes(t, u, v, dep, arr);

if(!ok)
{
    // 🔥 fallback (DO NOT SKIP)
    dep = 0;
    arr = 0;

    cout << "⚠️ Missing timing data → showing fallback\n";
}

static int variation = 0;

variation++;
int duration = arr - dep;

if(duration < 0)
    duration += 1440;

// distance-based rollover
int s=-1,e=-1;
for(int k=0;k<t.stations.size();k++)
{
    if(t.stations[k]==u) s=k;
    if(t.stations[k]==v){ e=k; break; }
}

if(s!=-1 && e!=-1 && e < t.distances.size())
{
    int dist = t.distances[e] - t.distances[s];

 //   if(dist >= 1200 && duration < 1440)
   //     duration += 1440;
}

    pureTravel += duration;

    if(j >= 1)
    {
//        int prevDep, prevArr;
       // getSegmentTimes(cur.path[j-1], stations[j-1], stations[j], prevDep, prevArr);
//        int currDep, currArr;
     //   getSegmentTimes(cur.path[j], stations[j], stations[j+1], currDep, currArr);
// previous segment
auto &prevSeg = cur.path[j-1];
Train prevT = prevSeg.first;
string pu = prevSeg.second.first;
string pv = prevSeg.second.second;

int prevDep, prevArr;
getSegmentTimes(prevT, pu, pv, prevDep, prevArr);

// current segment
auto &currSeg = cur.path[j];
Train currT = currSeg.first;
string cu = currSeg.second.first;
string cv = currSeg.second.second;

int currDep, currArr;
getSegmentTimes(currT, cu, cv, currDep, currArr);

        int prevAbs = prevArr;
        int currAbs = currDep;

        while(currAbs < prevAbs + LAYOVER)
            currAbs += 1440;

        totalLayover += (currAbs - prevAbs);
    }
}

long long realTotal = pureTravel + totalLayover;

journeys.push_back({cur.path, stations, realTotal});
    continue;

}
set<string> seen;
vector<Journey> unique;

for(auto &j : journeys)
{
    string key = "";

    for(auto &seg : cur.path)
    {
        key += seg.second.first + "->" + seg.second.second + "|";
    }

    if(seen.count(key)) continue;

    seen.insert(key);
    unique.push_back(j);
}

journeys = unique;


            string u = stationList[cur.node];

            if(!trainMatrix.count(u)) continue;

            for(auto &[v,trains] : trainMatrix[u])
            {
               // if(!pos.count(v)) continue;
                //if(pos[v] != pos[u]+1) continue; // strict Floyd
               if(!pos.count(u) || !pos.count(v)) continue;

              // if(pos[v] <= pos[u]) continue;
            //   if(pos[v] != pos[u] + 1) continue;
if(pos[v] <= pos[u]) continue;   // allow forward movement

//if(!pos.count(v)) continue;  /////////// 

if(!pos.count(v) || pos[v] != pos[u] + 1)
    continue;

                int vIdx = floydstationIndex[v];

                vector<pair<int,Train>> cand;

                for(auto &t:trains)
                {
                    int dep,arr;
                    if(!getSegmentTimes(t,u,v,dep,arr)) continue;

                   // int ready = cur.arrival;
                   // if(!cur.path.empty()) ready += LAYOVER;
/*
int ready = cur.arrival;

if(!cur.path.empty())
    ready += LAYOVER;  // mandatory 2hr buffer

int adjDep = dep;

// handle day rollover properly
while(adjDep < ready)
    adjDep += 1440;

int wait = adjDep - ready;
*/
int ready = cur.arrival;

int prevArrival = cur.arrival;
int nextDeparture = dep;

// enforce only constraint (NOT adding 2hr)
while(nextDeparture < prevArrival + LAYOVER)
    nextDeparture += 1440;

 //   if(nextDeparture - prevArrival < LAYOVER)
   // continue;
    //if(!cur.path.empty() && nextDeparture < prevArrival + LAYOVER)
    //continue;   // skip only bad ones
// actual layover = full gap
int wait = nextDeparture - prevArrival;

if(wait < 0)
    wait += 1440;
if(wait < 60) wait += 1440;  //////
// optional safety (long gap)
if(wait >= 0 && wait < LAYOVER)
    wait += 1440;


                    int adjDep = dep;
                    if(adjDep < ready%1440) adjDep += 1440;

                  //  int wait = adjDep - (ready%1440);

                    cand.push_back({wait,t});
                }

             //   sort(cand.begin(),cand.end());
/*
     sort(cand.begin(), cand.end(),
     [](const pair<int,Train> &a, const pair<int,Train> &b)
     {
         if(a.first != b.first)
             return a.first < b.first;

         return a.second.trainNumber < b.second.trainNumber; // tie-breaker
     });
*/
sort(cand.begin(), cand.end(),
[&](const pair<int,Train> &a, const pair<int,Train> &b)
{
    int depA, arrA, depB, arrB;

    getSegmentTimes(a.second, u, v, depA, arrA);
    getSegmentTimes(b.second, u, v, depB, arrB);

    int durA = arrA - depA;
    if(durA < 0) durA += 1440;

    int durB = arrB - depB;
    if(durB < 0) durB += 1440;

    int costA = a.first + durA;
    int costB = b.first + durB;

    return costA < costB;
});


                for(auto &[wait,t] : cand)
                {
                    if(wait > 36*60) continue;

                    int dep,arr;
                    getSegmentTimes(t,u,v,dep,arr);

                    if(arr < dep) arr += 1440;
          int duration = arr - dep;

if(duration < 0)
    duration += 1440;

// 🔥 SAME FIX AGAIN
//int segmentDistance = 0;

//if(segmentDistance >= 1200 && duration < 1440)
  //  duration += 1440;

//int dep,arr;
//getSegmentTimes(t,u,v,dep,arr);

//int duration = arr - dep;



                    State nxt;
                    nxt.node = vIdx;
                  //  nxt.arrival = arr;
                    //nxt.time = cur.time + wait + duration;

                    int absoluteArrival = cur.arrival + wait + duration;

                   // nxt.arrival = absoluteArrival;
                    nxt.arrival = cur.arrival + wait + duration;
                   // nxt.time = cur.time + wait + duration + 5 * cur.path.size();
//nxt.time = cur.time + wait + duration + 30 * cur.path.size();
nxt.time = cur.time + wait + duration;

                    nxt.path = cur.path;
                   // nxt.path.push_back(t);
                    nxt.path.push_back({t, {u, v}});

/*
                    if(cur.node == destIdx)
{
    if(cur.path.size() == stations.size()-1)   // ✅ EXACT MATCH
        journeys.push_back({cur.path, stations, cur.time});

    continue;

}
*/
                    pq.push(nxt);
                }
            }
        }

        if(journeys.empty())
{
    // fallback: allow relaxed constraints
    for(auto &t : trainMatrix[stationList[srcIdx]])
    {
        // optional simple fallback (skip strict rules)
    }
}

        for(auto &j:journeys)
            allJourneys.push_back(j);
    }

    if(allJourneys.empty()){
        cout<<"⚠️ No feasible routes found\n";
        return;
    }

    sort(allJourneys.begin(),allJourneys.end(),
         [](auto &a,auto &b){return a.totalTime < b.totalTime;});
/*
sort(allJourneys.begin(), allJourneys.end(),
[](const Journey &a, const Journey &b)
{
    if(a.totalTime != b.totalTime)
        return a.totalTime < b.totalTime;

    return a.trains.size() < b.trains.size(); // tie-breaker
});
*/
/*
set<string> seen;
vector<Journey> unique;

for(auto &j : allJourneys)
{
    string key = "";

    for(auto &t : j.trains)
        key += t.first.trainNumber + "-";

    if(!seen.count(key))
    {
        seen.insert(key);
        unique.push_back(j);
    }

    if(unique.size() == 10) break;
}

allJourneys = unique;
*/
vector<Journey> uniqueJourneys;
set<string> seen;

for(auto &j : allJourneys)
{
    string key = "";

    for(auto &t : j.trains)
        key += t.first.trainNumber + "-";

    if(seen.insert(key).second)
        uniqueJourneys.push_back(j);
}

// replace original
allJourneys = uniqueJourneys;



    int limit = min(10,(int)allJourneys.size());
    //int limit = min(10, (int)uniquePaths.size());

if(limit < 10)
    cout << "⚠️ Only " << limit << " valid unique routes found\n";

    cout<<"\n✨ Showing Top "<<limit<<" Smart Routes (Time Optimized)\n";


cout << "INDIRECT_START\n";

for(int i=0;i<limit;i++)
{
    auto &r = allJourneys[i];
    auto &stations = r.stations;

    long long totalLayover = 0;

    cout << "ROUTE_START\n";
    cout << "route:" << i+1 << "\n";

    if(i==0) cout << "tag:FASTEST\n";
    else cout << "tag:NORMAL\n";

    // 🔥 PATH
    cout << "path:";
    for(int j=0;j<stations.size();j++){
        cout<<getStationCity(stations[j]);
        if(j+1<stations.size()) cout<<"->";
    }
    cout << "\n";

    int limitSegments = min((int)r.trains.size(), (int)stations.size()-1);
    if(limitSegments <= 0) continue;

    long long pureTravel = 0;

    for(int j=0;j<limitSegments;j++)
    {
        // 🔥 LAYOVER
        if(j >= 1)
        {
            int prevDep, prevArr;
            int currDep, currArr;

            auto &prevSeg = r.trains[j-1];
            auto &currSeg = r.trains[j];

            Train prevT = prevSeg.first;
            Train currT = currSeg.first;

            string pu = prevSeg.second.first;
            string pv = prevSeg.second.second;

            string cu = currSeg.second.first;
            string cv = currSeg.second.second;

            getSegmentTimes(prevT, pu, pv, prevDep, prevArr);
            getSegmentTimes(currT, cu, cv, currDep, currArr);

            int prevAbs = prevArr;
            int currAbs = currDep;

            while(currAbs < prevAbs + LAYOVER)
                currAbs += 1440;

            int lay = currAbs - prevAbs;
            totalLayover += lay;

            cout << "LAYOVER:" 
                 << getStationCity(stations[j]) << ","
                 << lay/60 << "h " << lay%60 << "m\n";
        }

        // 🔥 TRAIN SEGMENT
        auto &seg = r.trains[j];
        Train t = seg.first;
        string u = seg.second.first;
        string v = seg.second.second;

        int dep, arr;
        getSegmentTimes(t, u, v, dep, arr);

        int duration = arr - dep;
        if(duration < 0) duration += 1440;

        pureTravel += duration;

        cout << "TRAIN_START\n";

        cout << "trainNo:" << t.trainNumber << "\n";
        cout << "name:" << t.trainName << "\n";
        cout << "from:" << u << "\n";
        cout << "to:" << v << "\n";
        cout << "departure:" << formatTime(dep) << "\n";
        cout << "arrival:" << formatTime(arr) << "\n";
        cout << "duration:" << duration/60 << "h " << duration%60 << "m\n";

        cout << "TRAIN_END\n";
    }

    long long totalTime = pureTravel + totalLayover;

    // 🔥 SUMMARY BLOCK
    cout << "SUMMARY_START\n";
    cout << "changes:" << (limitSegments-1) << "\n";
    cout << "travel:" << pureTravel/60 << "h " << pureTravel%60 << "m\n";
    cout << "layover:" << totalLayover/60 << "h " << totalLayover%60 << "m\n";
    cout << "total:" << totalTime/60 << "h " << totalTime%60 << "m\n";
    cout << "SUMMARY_END\n";

    cout << "ROUTE_END\n";
}

cout << "INDIRECT_END\n";

}

