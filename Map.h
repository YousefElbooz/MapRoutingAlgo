#
#define MAP_H

#include <vector>
#include <unordered_map>
#include <cmath>
#include <string>
#include <limits>
#include <stdexcept>

using namespace std;

// Represents a point of intersection on the map
struct Node {
    int id;         // Unique ID of the node (intersection)
    double x, y;    // Coordinates in 2D space

    
};

// Represents a road (edge) between two intersections
struct Edge {
    int from, to;           // IDs of the intersections this road connects
    double length;          // Road length in kilometers
    double speed = 0.0;     // Constant speed (used in normal case)

    vector<double> speedProfile;  // 1ST BONUS Optional: speeds in intervals (for bonus case)
    int speedInterval = 0;        // Optional: interval in minutes for changing speed

    // Calculate travel time in hours for the road based on time index
    double getTravelTime(int currentMinute = 0) const {
        if (!speedProfile.empty() && speedInterval > 0) {
            int index = (currentMinute / speedInterval) % speedProfile.size();
            return length / speedProfile[index];
        }
        return length / speed;
    }
};

// Represents a query with a source and destination point and walking limit
struct Query {
    double sourceX, sourceY;  // Real-world coordinates of the source
    double destX, destY;      // Real-world coordinates of the destination
    double R;                 // Maximum walking distance in meters
};

// Represents the result of a query/pathfinding operation
struct PathResult {
    vector<int> path;         // Sequence of intersection IDs visited
    double totalTime;         // Total time in minutes
    double totalDistance;     // Total distance in kilometers
    double walkingDistance;   // Total walking distance (source to node + node to dest)
    double vehicleDistance;   // Distance traveled in the vehicle

    PathResult() : totalTime(0), totalDistance(0),
        walkingDistance(0), vehicleDistance(0) {
    }
};

// Graph class to manage the full road network and operations
class Graph {
private:
    unordered_map<int, Node> nodes;   // Map of all nodes by ID
    vector<Edge> edges;               // All edges in the graph
    unordered_map<int, vector<int>> adjacencyList;  // Map: node ID -> edge indices

    bool bonusMode = false;  // If true, expect speedProfile input per edge (bonus mode)

public:
    Graph() = default;

    // -------------------- File I/O --------------------

    // Load map file (regular or bonus format) and build graph
    bool loadMap(const string& filename, bool bonus = false);

    // Load query file (source/destination + R per line)
    vector<Query> loadQueries(const string& filename);

    // Save results to output file in required format
    void saveResults(const string& filename, const vector<PathResult>& results);

    // -------------------- Graph Construction --------------------

    // Add a node (intersection) to the graph
    void addNode(const Node& node) {
        nodes[node.id] = node;
    }

    // Add a bidirectional edge between two nodes
    void addEdge(const Edge& edge) {
        int index = edges.size();
        edges.push_back(edge);
        adjacencyList[edge.from].push_back(index);

        // Add reverse edge for bidirectional roads
        Edge reverse = edge;
        swap(reverse.from, reverse.to);
        int revIndex = edges.size();
        edges.push_back(reverse);
        adjacencyList[reverse.from].push_back(revIndex);
    }

    // Clear graph (nodes, edges, adjacency list)
    void clear() {
        nodes.clear();
        edges.clear();
        adjacencyList.clear();
    }

    // -------------------- Graph Accessors --------------------

    const unordered_map<int, Node>& getNodes() const { return nodes; }
    const vector<Edge>& getEdges() const { return edges; }
    const unordered_map<int, vector<int>>& getAdjacencyList() const { return adjacencyList; }

    // Get node by ID (throws error if not found)
    const Node& getNode(int id) const {
        auto it = nodes.find(id);
        if (it == nodes.end()) {
            throw runtime_error("Node ID not found: " + to_string(id));
        }
        return it->second;
    }

    // Get edge by index (index from edges vector)
    const Edge& getEdge(int index) const {
        if (index < 0 || index >= edges.size()) {
            throw runtime_error("Invalid edge index");
        }
        return edges[index];
    }

    // Get edge indices connected to a given node
    const vector<int>& getAdjacentEdges(int nodeId) const {
        static const vector<int> empty;  // in case node has no edges
        auto it = adjacencyList.find(nodeId);
        return (it != adjacencyList.end()) ? it->second : empty;
    }

    size_t getNodeCount() const { return nodes.size(); }
    size_t getEdgeCount() const { return edges.size(); }

    // -------------------- Core Algorithms --------------------

    // Find all nodes within a given radius from (x, y) in meters
    vector<int> findNodesWithinRadius(double x, double y, double radius) const;

    // Find shortest path from one node to another (least time)
    PathResult findShortestPath(int startNode, int endNode) const;

    // Run full query: find best walking + vehicle path
    PathResult processQuery(const Query& query) const;

private:
    // Calculate straight-line distance (Euclidean) in meters
    double calculateDistance(double x1, double y1, double x2, double y2) const {
        double dx = x1 - x2;
        double dy = y1 - y2;
        return sqrt(dx * dx + dy * dy) * 1000.0; // km → m
    }

    // Calculate walking time in minutes
    double calculateWalkingTime(double distanceMeters) const {
        constexpr double walkingSpeedKmh = 5.0; // km/h
        double distanceKm = distanceMeters / 1000.0;
        return (distanceKm / walkingSpeedKmh) * 60.0;
    }
};

