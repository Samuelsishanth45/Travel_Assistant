#include "floyd_warshall.h"
#include "train_matrix.h"
#include "train.h"
#include "station_utils.h"
#include "csv_parser.h"
#include "station_mapper.h"
#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <limits>
#include <algorithm>
#include <ostream>
#include <set>
#include <climits>
#include <string>

using namespace std;

static const long long INF = 9e18;
static const int DAY = 1440;
static const int LAYOVER = 120;

static unordered_map<string, vector<int>> routeCache;

vector<vector<int>> kPaths;   // store multiple paths


vector<vector<RouteState>> distMatrix;
vector<string> stationList;
unordered_map<string,int> floydstationIndex;

vector<vector<int>> generateKPaths(int srcIdx, int destIdx, int K);
vector<int> extractPath(int srcIdx, int destIdx);
vector<int> getShortestPath(int srcIdx, int destIdx);
int toMin(const string &t)
{
    if(t.empty()) return -1;

    int h=0, m=0;

    if(sscanf(t.c_str(), "%d:%d", &h, &m) != 2)
        return -1;

    if(h < 0 || h > 23 || m < 0 || m > 59)
        return -1;

    return h*60 + m;
}

/* next departure >= required */
static int nextDeparture(int dep, int required){

    while(dep < required)
        dep += DAY;

    return dep;
}

static string normalize(const string &s){
    string r = s;
    transform(r.begin(), r.end(), r.begin(), ::toupper);
    r.erase(remove_if(r.begin(), r.end(), ::isspace), r.end());
    return r;
}


/* find earliest train from A->B after time T */
static bool findNextTrain(
    const vector<Train> &trains,
    long long arrivalTime,
    long long &newArrival,
    Train &chosen)
{
    long long best = INF;
    bool found=false;

    for(const auto &t : trains)
    {
        int dep = toMin(t.departure);
        int arr = toMin(t.arrival);

        if(dep < 0 || arr < 0)
            continue;

        long long baseDay = (arrivalTime / DAY) * DAY;
        long long depAbs = baseDay + dep;

        if(depAbs < arrivalTime)
            depAbs += DAY;

        long long arrAbs = depAbs + (arr - dep);

        if(arr < dep)
            arrAbs += DAY;

        if(arrAbs < best)
        {
            best = arrAbs;
            chosen = t;
            found=true;
        }
    }

    if(found)
        newArrival = best;

    return found;
}

/* ---------------- FLOYD ---------------- */

void computeTimeFloydMatrix(
        const string &src,
        const string &dest)
{
    stationList.clear();
    floydstationIndex.clear();

//auto reduced = reduceStations(src,dest);

 string key = src + "->" + dest;

    if(routeCache.count(key)){
        cout<<"⚡ Using cached Floyd route\n";
        return;
    }


unordered_set<string> reduced;
//queue<string> q;

//q.push(src);
//reduced.insert(src);

unordered_set<string> forward;
queue<string> q;

forward.insert(src);
q.push(src);

while(!q.empty())
//while(!q.empty() && forward.size() < 400)
{
    string u = q.front();
    q.pop();

    if(!trainMatrix.count(u))
        continue;

    for(auto &[v,_] : trainMatrix[u])
    {
        if(!forward.count(v))
        {
            forward.insert(v);
            q.push(v);
        }
    }
}

/* reverse BFS */

unordered_set<string> backward;
q.push(dest);
backward.insert(dest);

while(!q.empty())
//while(!q.empty() && backward.size() < 400)
{
    string u = q.front();
    q.pop();

    for(auto &[a,mp] : trainMatrix)
    {
        if(mp.count(u))
        {
            if(!backward.count(a))
            {
                backward.insert(a);
                q.push(a);
            }
        }
    }
}

//unordered_set<string> reduced;

for(auto &s : forward)
{
    if(backward.count(s))
        reduced.insert(s);
}


reduced.insert(dest); 

    /* ---- sort trains by departure ---- */

    for(auto &[u,mp] : trainMatrix)
    for(auto &[v,trains] : mp)
    {
        sort(trains.begin(), trains.end(),
            [](const Train &a,const Train &b){
              //  return a.departure < b.departure;

                return toMin(a.departure) < toMin(b.departure);
            });
    }

    /* ---- build station index ---- */

    for(const auto &s : reduced)
    {
        floydstationIndex[s] = stationList.size();
        stationList.push_back(s);
    }

    int N = stationList.size();

    cout<<"🔎 Reduced stations: "<<N<<endl;

    if(reduced.size() <= 1) {
    cout << "⚠️ No valid route found between stations.\n";
    return;
}

    distMatrix.assign(N, vector<RouteState>(N, RouteState(INF,-1)));

    for(int i=0;i<N;i++){
        distMatrix[i][i].totalTime = 0;
        distMatrix[i][i].next = i;
    }

    /* ---- debug ---- */

    if(floydstationIndex.count(src))
        cout<<"DEBUG: src index = "<<floydstationIndex[src]<<endl;

    if(floydstationIndex.count(dest))
        cout<<"DEBUG: dest index = "<<floydstationIndex[dest]<<endl;

    cout<<"DEBUG: trainMatrix size = "<<trainMatrix.size()<<endl;

    /* ---- DIRECT TRAIN EDGES ---- */

int directEdges = 0;

for(const auto &[u, mp] : trainMatrix)
{
    for(const auto &[v, trains] : mp)
    {
        if(!floydstationIndex.count(u) || !floydstationIndex.count(v))
            continue;

        int i = floydstationIndex[u];
        int j = floydstationIndex[v];

        int bestDist = INT_MAX;

        for(const Train &t : trains)
        {
            int s = -1, e = -1;

            for(int k = 0; k < t.stations.size(); k++)
            {
                if(t.stations[k] == u) s = k;
                if(t.stations[k] == v) { e = k; break; }
            }

            if(s == -1 || e == -1 || s >= e) continue;

            // 🔥 FIX: SAFE DISTANCE CALC
            if(e < t.distances.size() && s < t.distances.size())
            {
                int dist = t.distances[e] - t.distances[s];

                if(dist <= 0) continue;   // keep only valid

                bestDist = min(bestDist, dist);
            }
        }

        if(bestDist == INT_MAX) continue;

        distMatrix[i][j].totalTime = bestDist;
        distMatrix[i][j].next = j;
        directEdges++;
    }
}

cout << "DEBUG directEdges = " << directEdges << endl;


    /*
  int directEdges = 0;

for(const auto &[u, mp] : trainMatrix)
{
    for(const auto &[v, trains] : mp)
    {
        if(!floydstationIndex.count(u) ||
           !floydstationIndex.count(v))
            continue;

        int i = floydstationIndex[u];
        int j = floydstationIndex[v];

        // use MIN distance among trains

        int bestDist = INT_MAX
for(const Train &t : trains)
{
    int s = -1, e = -1;

    for(int k = 0; k < t.stations.size(); k++)
    {
        if(t.stations[k] == u) s = k;
        if(t.stations[k] == v) { e = k; break; }
    }

    if(s == -1 || e == -1 || s >= e) continue;

    int dist = t.distances[e] - t.distances[s];

    if(dist <= 0) continue;

    bestDist = min(bestDist, dist);
}

if(bestDist == INT_MAX) continue;

if(bestDist < distMatrix[i][j].totalTime)
{
    distMatrix[i][j].totalTime = bestDist;
    distMatrix[i][j].next = j;
    directEdges++;
}
    }
}
cout << "DEBUG directEdges = " << directEdges << endl;
  */
/////////////////////////////////////////
/*
for(const auto &[u, mp] : trainMatrix)
for(const auto &[v, trains] : mp)
{
    if(!floydstationIndex.count(u) ||
       !floydstationIndex.count(v))
        continue;

    int i = floydstationIndex[u];
    int j = floydstationIndex[v];
    for(const Train &t : trains)
    {
        if(t.departureTimes.empty() || t.arrivalTimes.empty())
            continue;

      //  int dep = toMin(t.departureTimes[0]);
        //int arr = toMin(t.arrivalTimes.back());

        int s = -1, e = -1;

for(int k = 0; k < t.stations.size(); k++) {
    if(t.stations[k] == u) s = k;
    if(t.stations[k] == v) { e = k; break; }
}

if(s == -1 || e == -1 || s >= e)
    continue;

int dep = toMin(t.departureTimes[s]);
int arr = toMin(t.arrivalTimes[e]);

        if(dep < 0 || arr < 0)
            continue;

long long travel = arr - dep;

if(travel < 0)
    travel += 1440;

// 🔥 FIX: multi-day correction using ACTUAL duration
int segmentDistance = 0;

if(e < t.distances.size() && s < t.distances.size())
    segmentDistance = t.distances[e] - t.distances[s];

if(segmentDistance > 0)
{
    double speed = (double)segmentDistance / (travel / 60.0);

    if(speed > 120 || speed < 10)
        continue;
}

// average train speed ~50 km/h → estimate days
int expectedMinutes = segmentDistance * 60 / 50;

while(travel < expectedMinutes)
{
    travel += 1440;
}

if(travel < 30 || travel > 5000)
    continue;

    if(segmentDistance < 50) continue;

if(u == "CSMT" && v == "SBC")
{
    cout<<"DEBUG CSMT->SBC travel = "<<travel<<endl;
}


////////////////////////////////////////////
long long travel = arr - dep;

if(travel < 0)
    travel += 1440;

int segmentDistance = 0;

if(e < t.distances.size() && s < t.distances.size())
    segmentDistance = t.distances[e] - t.distances[s];

if(segmentDistance >= 1200 && travel < 1440)
{
    travel += 1440;
}


if(travel <= 0 || travel > 3000)
    continue;
*/

/*
long long travel = arr - dep;

if(travel < 0)
    travel += 1440;

if(t.distances.size() >= 2 &&
   (t.distances.back() - t.distances[0]) >= 1200 &&
   travel < 1440)
{
    travel += 1440;
}


        if(arr < dep) travel += 1440;
        travel += LAYOVER;
//////////////////////////////////////////////////////////
        if(travel < distMatrix[i][j].totalTime)
        {
distMatrix[i][j].totalTime = travel;
distMatrix[i][j].next = j;
            directEdges++;
        }
    }    
}
*/
if(directEdges < 500)
{
    cout<<"⚠️ Graph too sparse — check edge filters\n";
}

cout<<"DEBUG: Matrix allocated successfully, size = "<<N<<"x"<<N<<endl;

cout<<"DEBUG directEdges = "<<directEdges<<endl;

    /* ---- FLOYD RELAXATION ---- */

int srcIdx = floydstationIndex[src];
int destIdx = floydstationIndex[dest];

//kPaths.clear();

/*
for(int k=0;k<N;k++)
for(int i=0;i<N;i++)

{
    
    if(distMatrix[i][k].totalTime == INF) continue;

    for(int j=0;j<N;j++)
    {
        if(distMatrix[k][j].totalTime == INF) continue;
        
/////////////////
if(i == j) continue;

int next_i_k = distMatrix[i][k].next;
int next_k_j = distMatrix[k][j].next;

// 🚫 prevent loops / going back
if(next_i_k == -1 || next_k_j == -1)
    continue;

if(next_i_k == j)   // direct loop
    continue;
    ////////////////////////

        long long newTime =
            distMatrix[i][k].totalTime +
            distMatrix[k][j].totalTime;
          //  LAYOVER;

        if(newTime < distMatrix[i][j].totalTime)
        {
            distMatrix[i][j].totalTime = newTime;
            distMatrix[i][j].next = distMatrix[i][k].next;
        }
    }
    


}
*/
/*
for(int k=0;k<N;k++)
{
    for(int i=0;i<N;i++)
    {
        if(distMatrix[i][k].totalTime == INF) continue;

        for(int j=0;j<N;j++)
        {
            if(distMatrix[k][j].totalTime == INF) continue;

            if(distMatrix[i][k].next == -1) continue;

            long long newTime =
                distMatrix[i][k].totalTime +
                distMatrix[k][j].totalTime;

            if(newTime < distMatrix[i][j].totalTime)
            {
                distMatrix[i][j].totalTime = newTime;
               // distMatrix[i][j].next = distMatrix[i][k].next;
             if(distMatrix[i][k].next != -1)
    distMatrix[i][j].next = distMatrix[i][k].next;
            }
        }
    }
} 
*/

for(int k=0;k<N;k++)
{
    for(int i=0;i<N;i++)
    {
        if(distMatrix[i][k].totalTime == INF) continue;

        for(int j=0;j<N;j++)
        {
            if(distMatrix[k][j].totalTime == INF) continue;

            long long newDist =
                distMatrix[i][k].totalTime +
                distMatrix[k][j].totalTime;

           if(newDist < distMatrix[i][j].totalTime)
{
    if(distMatrix[i][k].next == -1) continue;

    distMatrix[i][j].totalTime = newDist;
    distMatrix[i][j].next = distMatrix[i][k].next;
}
        }
    }
}

cout<<"DEBUG: Floyd completed"<<endl;

for(int i=0;i<N;i++)
{
    for(int j=0;j<N;j++)
    {
        if(distMatrix[i][j].totalTime < INF && i != j)
        {
            if(distMatrix[i][j].next == -1)
            {
                distMatrix[i][j].next = j;
            }
        }
    }
}

cout<<"DEBUG: final distance = "
    <<distMatrix[srcIdx][destIdx].totalTime<<endl;

cout<<"DEBUG: next pointer = "
    <<distMatrix[srcIdx][destIdx].next<<endl;

    vector<int> basePath = extractPath(srcIdx, destIdx);

// ✅ LIMIT HOPS (max 5 edges = 6 stations)
if(basePath.size() > 6)
{
    basePath.resize(6);
}
//if(basePath.size() < 2)
  //  return;

    cout<<"DEBUG: base path size = "<<basePath.size()<<endl;

cout << "\n===== FINAL FLOYD PATH =====\n";

//vector<int> basePath = extractPath(srcIdx, destIdx);
basePath = extractPath(srcIdx, destIdx);

if(basePath.empty())
{
    cout << "❌ NO PATH\n";
}
else
{
    for(int x : basePath)
        cout << stationList[x] << " -> ";
    cout << endl;
}


vector<int> compressed;

compressed.push_back(basePath[0]);

for(int i=1;i<basePath.size()-1;i++)
{
    int u = basePath[i-1];
    int v = basePath[i];
    int w = basePath[i+1];

    // skip unnecessary intermediate nodes
    if(distMatrix[u][w].totalTime <= 
       distMatrix[u][v].totalTime + distMatrix[v][w].totalTime)
        continue;

    compressed.push_back(v);
}

compressed.push_back(basePath.back());

basePath = compressed;

/*
for(int i=0;i<N;i++)
{
    for(int j=0;j<N;j++)
    {
        if(distMatrix[i][j].totalTime < INF && distMatrix[i][j].next == -1)
        {
            distMatrix[i][j].next = j;
        }
    }
}
*/


//kPaths = generateKPaths(srcIdx, destIdx, 30);

//cout<<"✅ Generated "<<kPaths.size()<<" Floyd-based routes\n";
    // Extract path
  //  vector<int> path = extractPath(srcIdx, destIdx);

    //if(path.empty()) break;

    //kPaths.push_back(path);

  
//}

//kPaths = generateKPaths(srcIdx, destIdx, 30);


//cout<<"✅ Generated "<<kPaths.size()<<" Floyd routes\n";


//vector<int> basePath = extractPath(srcIdx, destIdx);

cout<<"DEBUG: base path size = "<<basePath.size()<<endl;

kPaths.clear();

//kPaths = generateKPaths(srcIdx, destIdx, 30);
kPaths = generateKPaths(srcIdx, destIdx, 50);

cout<<"DEBUG: total kPaths = "<<kPaths.size()<<endl;
//scout<<"✅ Generated "<<kPaths.size()<<" Floyd-based routes\n";

if(distMatrix[srcIdx][destIdx].totalTime >= INF)
{
    cout<<"❌ No path exists in Floyd\n";
    return;
}
}

vector<vector<int>> generateKPaths(int srcIdx, int destIdx, int K)
{

    cout<<"DEBUG: ENTER generateKPaths"<<endl;
    vector<vector<int>> paths;

    vector<int> basePath = extractPath(srcIdx, destIdx);

        if(basePath.empty())
{
    cout<<"❌ ERROR: basePath is empty\n";
    return paths;
}

    // ✅ ALWAYS include base path
    paths.push_back(basePath);

// 🔥 ADD RANDOM NEIGHBOR PATHS
for(auto &[v,_] : trainMatrix[stationList[srcIdx]])
{
    if(!floydstationIndex.count(v)) continue;

    int vIdx = floydstationIndex[v];

    vector<int> tail = extractPath(vIdx, destIdx);
    if(tail.empty()) continue;

    vector<int> newPath;
    newPath.push_back(srcIdx);
    newPath.push_back(vIdx);

    for(int j=1;j<tail.size();j++)
        newPath.push_back(tail[j]);

    if(newPath.size() <= 6)
        paths.push_back(newPath);
}


    int L = basePath.size();

    // 🔥 Try simple variations (skip nodes)
    for(int skip = 1; skip < L-1 && paths.size() < K; skip++)
    {
        vector<int> newPath;

        // take prefix
        for(int i=0;i<skip;i++)
            newPath.push_back(basePath[i]);

        int u = basePath[skip-1];

        // try neighbors of u
        if(!trainMatrix.count(stationList[u])) continue;

        for(auto &[alt, _] : trainMatrix[stationList[u]])
        {
            if(!floydstationIndex.count(alt)) continue;

            int altIdx = floydstationIndex[alt];

            if(altIdx == basePath[skip]) continue;

            vector<int> tail = extractPath(altIdx, destIdx);

            if(tail.empty()) continue;


            /*
            vector<int> candidate = newPath;

            candidate.push_back(altIdx);

            for(int j=1;j<tail.size();j++)
                candidate.push_back(tail[j]);

            if(candidate.size() > 6)   // max 5 hops
    continue;


set<int> visited;
bool cycle = false;

for(int x : candidate)
{
    if(visited.count(x))
    {
        cycle = true;
        break;
    }
    visited.insert(x);
}

if(!cycle)
  //  paths.push_back(newPath);


            paths.push_back(candidate);
*/
vector<int> candidate = newPath;

candidate.push_back(altIdx);

for(int j=1;j<tail.size();j++)
    candidate.push_back(tail[j]);

// ✅ limit hops (max 5 edges = 6 nodes)
if(candidate.size() > 6)
    continue;

// cycle check
set<int> visited;
bool cycle = false;

for(int x : candidate)
{
    if(visited.count(x))
    {
        cycle = true;
        break;
    }
    visited.insert(x);
}

if(!cycle)
    paths.push_back(candidate);

            if(paths.size() >= K)
                break;
        }
    }

    // ✅ SAFETY: if only base path exists
    if(paths.size() == 1)
    {
        // duplicate base path slightly (to allow PQ exploration)
        for(int i=0;i<min(K-1,3);i++)
            paths.push_back(basePath);
    }

    if(paths.empty())
{
    vector<int> basePath = extractPath(srcIdx, destIdx);
    if(!basePath.empty())
        paths.push_back(basePath);
}

    return paths;

}

vector<int> extractPath(int u, int v)
{
    vector<int> path;

    if(distMatrix[u][v].next == -1)
        return path;

    path.push_back(u);

    while(u != v)
    {
        u = distMatrix[u][v].next;

        if(u == -1)   // 🔥 safety
        {
            cout << "❌ Broken next pointer\n";
            return {};
        }

        path.push_back(u);

        if(path.size() > 100) break; // 🔥 avoid infinite loop
    }

    return path;
}

/*
vector<int> extractPath(int srcIdx, int destIdx)
{
    vector<int> path;

    int curr = srcIdx;

    int safety = 0;

    while(curr != destIdx && safety < 1000)
    {
        path.push_back(curr);

        int nxt = distMatrix[curr][destIdx].next;
if(nxt == -1)
{
    cout<<"❌ Broken next pointer at "<<curr<<endl;
    path.clear();
    return path;
}



        curr = nxt;
        safety++;
    }

    if(curr == destIdx)
        path.push_back(destIdx);
    else
        path.clear();

    return path;
}
    */