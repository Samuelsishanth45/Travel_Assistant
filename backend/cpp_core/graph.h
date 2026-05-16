#ifndef GRAPH_H
#define GRAPH_H

#include <string>
#include <vector>

// Forward declaration instead of including csv_parser.h
struct Train;

struct Connection {
    std::string station;
    int duration;
};

struct GraphNode;

/*
struct GraphNode {
    std::string station;
    std::vector<Connection> neighbors;
};
*/
// Function declarations
std::map<std::string, GraphNode*> buildGraph(const std::vector<Train>& trains);
void cleanupGraph(std::map<std::string, GraphNode*>& graph);

#endif


