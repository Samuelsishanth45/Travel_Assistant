// journey_planner.h
#ifndef JOURNEY_PLANNER_H
#define JOURNEY_PLANNER_H

#include <string>
#include <vector>
#include "train.h"
#include "maps_api.h"

// A single canonical JourneyStep struct used across the project.
struct JourneyStep {
    // short transport mode: "TRAIN", "LOCAL", "DRIVE", "WALK", etc.
    std::string mode;

    // human-friendly vehicle text like "12345 - Rajdhani"
    std::string vehicle;

    // generic from/to labels
    std::string from;
    std::string to;

    // instruction text for user
    std::string instruction;

    // distance in km
    double distanceKm = 0.0;

    // duration in seconds
    unsigned long durationSec = 0;

    // departure / arrival human times
    std::string departTime;
    std::string arriveTime;

    // maps url or additional link
    std::string mapsUrl;
};

struct PlannerStep {
    std::string mode, service, from, to;
    std::string time, duration, distance, note;
};

struct JourneyPlan {
    std::vector<PlannerStep> steps;
    std::string totalTime;
    std::string totalDistance;
};


namespace journey_planner {

    void displayPlannerAPI(const JourneyPlan &plan);

std::vector<JourneyStep> buildDirectFirstPlan(const std::string& origin,
                                              const std::string& dest,
                                              const std::vector<Train>& allTrains);

std::vector<JourneyStep> buildMixedBestPlan(const std::string& origin,
                                            const std::string& dest,
                                            const std::vector<Train>& allTrains);
/*
std::vector<JourneyStep> getRealTimeJourneyPlan(const std::string& source,
                                                const std::string& destination,
                                                const std::vector<Train>& allTrains,
                                                size_t numCandidateStations = 3);
*/
std::vector<JourneyStep> getRealTimeJourneyPlan(
    const std::string& source,
    const std::string& destination,
    size_t numCandidateStations);


void displayRealTimeJourney(const std::vector<JourneyStep>& steps,
                            const std::string& origin,
                            const std::string& destination);

   std::vector<JourneyStep> buildAIMixedPlan(
    const std::string &origin,
    const std::string &dest,
    const std::vector<Train> &allTrains


);
 long parseTimeHHMMSS(const std::string& t);
DistanceMatrixResult guardedDistanceMatrix(const std::string& from, const std::string& to);
DistanceMatrixResult estimateFallback(double km);

JourneyStep buildLocalStep(const std::string &from,
                               const std::string &to,
                               unsigned long durSec,
                               double km,
                               const std::string &note = "");

} // namespace journey_planner

// old free functions used by some .cpp files (kept for compatibility)
std::vector<JourneyStep> buildDirectBest(const std::string& origin,
                                         const std::string& dest,
                                         const std::vector<Train>& allTrains);

std::vector<JourneyStep> buildMixedShortest(const std::string& origin,
                                            const std::string& dest,
                                            const std::vector<Train>& allTrains);

void displayTwoPlans(const std::vector<JourneyStep>& mixed,
                     const std::vector<JourneyStep>& direct,
                     const std::string& origin,
                     const std::string& dest);



#endif // JOURNEY_PLANNER_H
