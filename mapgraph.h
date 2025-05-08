#ifndef MAPGRAPH_H
#define MAPGRAPH_H

#include <vector>
#include <unordered_map>
#include <string>
#include <limits>
#include <queue>
#include <tuple>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>

struct Node {
    int id;
    double x;
    double y;
};

struct Edge {
    int source;
    int destination;
    double distance;
    double speed;
};

struct PathResult {
    std::vector<int> path;
    double travelTime;
    double distance;
    double directDistance;
    std::string resultText;
};

class MapGraph {
public:
    MapGraph();
    ~MapGraph();

    bool loadMapFromFile(const std::string& filename);
    bool loadQueriesFromFile(const std::string& filename);
    std::string findShortestPath(double startX, double startY, double endX, double endY, double maxSpeed);
    std::string compareWithOutput(const std::string& outputFilename);
    std::vector<PathResult> runAllQueries();
    
    // Getters for visualization
    const std::vector<Node>& getNodes() const { return nodes; }
    const std::vector<Edge>& getEdges() const { return edges; }
    const std::vector<int>& getLastPath() const { return lastPath; }
    
    // Get closest node to coordinates
    int findClosestNode(double x, double y) const;

private:
    std::vector<Node> nodes;
    std::vector<Edge> edges;
    std::vector<std::vector<std::pair<int, double>>> adjacencyList; // node -> [(neighbor, travel_time)]
    
    // For faster lookups
    std::unordered_map<std::string, int> edgeMap; // "source_dest" -> edge index
    std::unordered_map<int, std::pair<double, double>> nodePositions; // node id -> (x, y)
    
    // For queries
    struct Query {
        double startX, startY;
        double endX, endY;
        double maxSpeed;
    };
    std::vector<Query> queries;
    
    // For result tracking
    std::vector<int> lastPath;
    double lastTravelTime = 0.0;
    double lastDistance = 0.0;
    double lastDirectDistance = 0.0;
    
    // Helper methods
    double calculateDistance(double x1, double y1, double x2, double y2) const;
};

#endif // MAPGRAPH_H 