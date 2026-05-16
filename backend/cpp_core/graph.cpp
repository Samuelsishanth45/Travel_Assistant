#include "graph.h"
#include "maps_api.h"
#include <iostream>
#include <vector>

using namespace std;
#if 0

map<string, GraphNode*> buildGraph(const vector<Train>& trains) {
    // Not using graph structure anymore
    return {};
}

vector<Route> findSmartRoutes(const vector<Train>& trains,
                            const string& source,
                            const string& destination) {
    
    vector<Route> routes;
    
    cout << "🎯 Finding smart routes using Google Maps..." << endl;
    
    // Use Google Maps to find alternative routes
    auto googleRoutes = findTrainRoutes(source, destination);
    
    if (googleRoutes.empty()) {
        cout << "❌ No routes found between " << source << " and " << destination << endl;
        return routes;
    }
    
    // Convert Google Maps routes to our format
    for (size_t i = 0; i < googleRoutes.size(); i++) {
        Route route;
        route.journeyTime = googleRoutes[i].duration;
        route.totalDistance = 0; // We'll calculate from trains
        
        // Add stations from the route
        if (!googleRoutes[i].trains.empty()) {
            route.stations.push_back(googleRoutes[i].trains[0].departureStation);
            for (const auto& train : googleRoutes[i].trains) {
                route.stations.push_back(train.arrivalStation);
                route.trains.push_back(train.trainName);
            }
        }
        
        route.transferCount = googleRoutes[i].trains.size() - 1;
        routes.push_back(route);
    }
    
    return routes;
}

void displayRouteDetails(const Route& route, const vector<Train>& trains) {
    cout << "\n" << string(60, '=') << endl;
    cout << "🚂 JOURNEY PLAN" << endl;
    cout << string(60, '=') << endl;
    
    cout << "📊 Journey Summary:" << endl;
    cout << "   • Estimated Time: " << route.journeyTime << endl;
    cout << "   • Transfers: " << route.transferCount << endl;
    cout << "   • Route Type: Google Maps Recommended" << endl;
    cout << endl;
    
    cout << "🗺️  Your Travel Plan:" << endl;
    for (size_t i = 0; i < route.stations.size() - 1; i++) {
        cout << "   " << (i + 1) << ". ";
        
        if (i < route.trains.size()) {
            cout << "🚆 " << route.trains[i] << endl;
            cout << "      " << route.stations[i] << " → " << route.stations[i + 1] << endl;
            
            if (i < route.stations.size() - 2) {
                cout << "      ⏸️  Transfer at " << route.stations[i + 1] << endl;
            }
        }
        cout << endl;
    }
    
    cout << string(60, '=') << endl;
}

void findAllRoutes(map<string, GraphNode*>& graph, 
                  const vector<Train>& trains,
                  const string& source, 
                  const string& destination) {
    
    cout << "\n🎯 Smart Route Planning: " << source << " → " << destination << endl;
    
    vector<Route> routes = findSmartRoutes(trains, source, destination);
    
    if (routes.empty()) {
        cout << "\n❌ No practical routes found between " << source << " and " << destination << endl;
        cout << "\n💡 Travel Tips:" << endl;
        cout << "   1. Check if stations are correctly identified" << endl;
        cout << "   2. Try using major city names" << endl;
        cout << "   3. Use Option 6 for Google Maps directions" << endl;
        return;
    }
    
    cout << "\n✅ Found " << routes.size() << " route(s) from Google Maps" << endl;
    
    // Show all found routes
    for (size_t i = 0; i < routes.size(); i++) {
        cout << "\n🏆 Route " << (i + 1) << " of " << routes.size() << ":" << endl;
        displayRouteDetails(routes[i], trains);
    }
}

void cleanupGraph(map<string, GraphNode*>& graph) {
    // Nothing to cleanup
}
#endif
