#ifndef MAPGRAPH_H
#define MAPGRAPH_H

#include <vector>
#include <string>
#include <queue>
#define priorityQueue std::priority_queue<std::pair<double, int>, std::vector<std::pair<double, int>>, std::greater<>>

struct Node {
    int id;
    double x;
    double y;
};

struct Edge {
    double distance;
    double speed;
};

// For queries
struct Query {
    double startX, startY;
    double endX, endY;
    double R;
};

struct PathResult {
    std::vector<int> path;
    double travelTime;
    double totalDistance;
    double walkingDistance;
    double vehicleDistance;
    std::string resultText;
};

class MapGraph {
public:
    MapGraph();
    ~MapGraph();
    void clearLastPath();

    bool empty();
    bool loadMapFromFile(const std::string& filename);
    bool loadQueriesFromFile(const std::string& filename);
    PathResult findShortestPath(double startX, double startY, double endX, double endY, double R);
    std::string displayOutput(const std::vector<PathResult> &results) const;

    std::vector<std::pair<int, double>> findNodesWithinRadius(double x, double y, double R, std::priority_queue<std::pair<double, int>, std::vector<std::
                                                              pair<double, int>>, std::greater<>> &pq, std::vector<double> &time, std::vector<double> &dist) const;

    std::vector<PathResult> runAllQueries();

    // Get nodes and edges
    std::vector<std::pair<double, double>> getNodes(){return nodePositions;}
    std::vector<std::pair<int,int>> getEdges(){return edges;}
    
    // Getters for visualization
    const std::vector<int>& getLastPath() const { return lastPath; }
    const std::vector<Query>& getQueries() const { return queries; }
    
    // Get closest node to coordinates
    std::pair<int,double> findClosestNode(double x, double y) const;

private:
    std::vector<std::vector<std::pair<int, Edge>>> adjacencyList; // node -> [(neighbor, travel_time)]
    double max_speed;

    // For faster lookups
    std::vector<std::pair<int,int>> edges;
    std::vector<std::pair<double, double>> nodePositions; // node id -> (x, y)

    std::vector<Query> queries;
    
    // For result tracking
    std::vector<int> lastPath;

    // Helper methods
    static double calculateDistance(double x1, double y1, double x2, double y2) ;
};

#endif // MAPGRAPH_H
