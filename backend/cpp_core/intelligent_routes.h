// intelligent_routes.h
#ifndef INTELLIGENT_ROUTES_H
#define INTELLIGENT_ROUTES_H

#include <string>
#include <vector>
#include "journey_planner.h"
#include "train.h"

// A simple TrainJourney structure used by intelligent_routes.cpp
struct TrainJourney {
    std::vector<JourneyStep> steps;
    unsigned long totalDuration = 0;
    double totalDistance = 0.0;
    int transferCount = 0;
};

std::vector<TrainJourney> findIntelligentRoutes(const std::string& origin,
                                                const std::string& destination);

void displayJourneyPlan(const TrainJourney& journey, int id);

#endif // INTELLIGENT_ROUTES_H



