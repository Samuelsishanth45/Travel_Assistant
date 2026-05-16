#ifndef CSV_PARSER_H
#define CSV_PARSER_H


#include <string>
#include <vector>
#include "train.h"


// Load trains from a CSV file (declaration)
std::vector<Train> loadTrainsFromJSON(const std::string& filename);

std::vector<Train> loadTrainsFromCSV(const std::string& filename);


#endif // CSV_PARSER_H









/*
struct Train {
    std::string trainNumber;
    std::string trainName;
    std::string source;
    std::string destination;
    int duration;
    int distance;
    std::string departure;
    std::string arrival;
    std::string zone;
    std::string type;
    bool virtualTrain;
    
    // Constructor with defaults
    Train() : duration(0), distance(0), virtualTrain(false) {}
};

// Function declarations
std::vector<Train> loadTrainsFromCSV(const std::string& filename);
std::vector<Train> loadTrainsFromJSON(const std::string& filename);  // ADD THIS LINE

#endif
*/