#ifndef TOURIST_H
#define TOURIST_H

#include <string>

using namespace std;

void displayTouristAttractions(const std::string& city);
void displayPopularCircuits();
//void generateSmartTravelPlan(string city, int days);
//string formatStationName(string name);
void planSmartTour(const string& city, int days);


#endif






/*
#ifndef TOURIST_H
#define TOURIST_H

#include <string>
#include <vector>

void displayTouristAttractions(const std::string& city);
void displayPopularCircuits();

vector<Attraction> fetchAttractions(const std::string& city,
                                    bool bestTimeMode = false);


//std::vector<std::string> fetchAttractionsFromGemini(const std::string& city);

int getDistanceFromCity(const std::string& city,
                        const std::string& place);


#endif
*/

