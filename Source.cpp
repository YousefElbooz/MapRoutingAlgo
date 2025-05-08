#include "map.h"
#include <iostream>

using namespace std;

int main() {
    Graph graph;

    // === LOAD MAP ===
    string mapFile = "map1.txt";
    bool isBonus = false;  // Set to true for bonus format map
    if (!graph.loadMap(mapFile, isBonus)) {
        return 1; // Failed
    }
    cout << "Map loaded: " << graph.getNodeCount() << " nodes, " << graph.getEdgeCount() << " edges\n";

    // === LOAD QUERIES ===
    string queryFile = "queries1.txt";
    vector<Query> queries = graph.loadQueries(queryFile);
    cout << "Loaded " << queries.size() << " queries.\n";

    // === PROCESS EACH QUERY ===
    vector<PathResult> results;
    for (const auto& q : queries) {
        // Dummy result until pathfinding is implemented
        PathResult res;
        res.path = { 0, 1, 2 }; // placeholder
        res.totalTime = 4.63;
        res.totalDistance = 1.72;
        res.walkingDistance = 0.28;
        res.vehicleDistance = 1.44;
        results.push_back(res);
    }

    // === SAVE OUTPUT ===
    graph.saveResults("output1.txt", results);
    cout << "Results saved to output1.txt\n";

    return 0;
}
