#include "mapgraph.h"
#include "qminmax.h"
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <cmath>
#include <sstream>

MapGraph::MapGraph() = default;

MapGraph::~MapGraph() = default;

bool MapGraph::empty() const {
    return adjacencyList.empty();
}

bool MapGraph::loadMapFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening map file: " << filename << std::endl;
        return false;
    }

    try {
        // Clear previous data
        nodePositions.clear();
        edges.clear();
        adjacencyList.clear();
    
        // Read the number of nodes
        int numNodes;
        file >> numNodes;
        
        if (numNodes <= 0 || numNodes > 1000000) { // Sanity check for node count
            std::cerr << "Invalid number of nodes: " << numNodes << std::endl;
            return false;
        }
        
        // Reserve capacity to avoid reallocations
        nodePositions.reserve(numNodes);

        // Read node information
        for (int i = 0; i < numNodes; i++) {
            Node node{};
            file >> node.id >> node.x >> node.y;
            
            // Check for invalid node data
            if (file.fail()) {
                std::cerr << "Error reading node data at index " << i << std::endl;
                return false;
            }
            
            // Create the spatial index for faster lookups
            nodePositions.emplace_back(node.x, node.y);
        }
        
        // Read the number of edges
        int numEdges;
        file >> numEdges;

        if (numEdges <= 0 || numEdges > 10000000) { // Sanity check for edge count
            std::cerr << "Invalid number of edges: " << numEdges << std::endl;
            return false;
        }

        // Reserve capacity
        edges.reserve(numEdges);

        // Initialize the adjacency list
        adjacencyList.resize(numNodes);

        max_speed = 0;
        // Read edge information
        for (int i = 0; i < numEdges; i++) {
            int source, destination;
            Edge edge{};
            file >> source >> destination >> edge.distance >> edge.speed;

            max_speed = qMax(max_speed, edge.speed);

            // Check for invalid edge data
            if (file.fail()) {
                std::cerr << "Error reading edge data at index " << i << std::endl;
                return false;
            }

            edges.emplace_back(source,destination);

            // Update adjacency list
            adjacencyList[source].push_back({destination, {edge.distance, edge.speed}});
            // Assuming bidirectional edges
            adjacencyList[destination].push_back({source, {edge.distance, edge.speed}});

        }
        
        file.close();
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception during map loading: " << e.what() << std::endl;
        file.close();
        return false;
    }
}

bool MapGraph::loadQueriesFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening queries file: " << filename << std::endl;
        return false;
    }
    
    try {
        // Clear previous queries
        queries.clear();
        
        // Read the number of queries
        int numQueries;
        file >> numQueries;
        
        if (numQueries <= 0 || numQueries > 100000) { // Sanity check for query count
            std::cerr << "Invalid number of queries: " << numQueries << std::endl;
            return false;
        }
        
        // Reserve capacity
        queries.reserve(numQueries);
        
        // Read query information
        for (int i = 0; i < numQueries; i++) {
            Query q{};
            file >> q.startX >> q.startY >> q.endX >> q.endY >> q.R;
            q.R/=1000; // Convert to km

            // Check for invalid query data
            if (file.fail()) {
                std::cerr << "Error reading query data at index " << i << std::endl;
                return false;
            }
            
            // Validate max speed
            if (q.R < 0) {
                std::cerr << "Invalid max walking distance in query " << i << ": " << q.R << std::endl;
                q.R = 0; // Use default instead
            }
            
            queries.push_back(q);
        }
        
        file.close();
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception during query loading: " << e.what() << std::endl;
        file.close();
        return false;
    }
}

PathResult MapGraph::findShortestPath(double startX, double startY, double endX, double endY, double R) {
    // Priority queue for Dijkstra's algorithm - (distance, node)
    priorityQueue pqForward;
    priorityQueue pqBackward;

    // Initialize arrays for Dijkstra's algorithm
    std::vector timeForward(nodePositions.size(), std::numeric_limits<double>::infinity());
    std::vector timeBackward(nodePositions.size(), std::numeric_limits<double>::infinity());
    std::vector distForward(nodePositions.size(), 0.0);
    std::vector distBackward(nodePositions.size(), 0.0);
    std::vector prevForward(nodePositions.size(), -1);
    std::vector prevBackward(nodePositions.size(), -1);
    std::vector visitedStart(nodePositions.size(), false);
    std::vector visitedEnd(nodePositions.size(), false);

    // Find the closest nodes to start and end coordinates
    std::vector<std::pair<int, double>> startNodes = findNodesWithinRadius(startX, startY, R, pqForward, timeForward, distForward);
    std::vector<std::pair<int, double>> endNodes = findNodesWithinRadius(endX, endY, R, pqBackward, timeBackward, distBackward);

    PathResult result;
    result.travelTime = std::numeric_limits<double>::infinity();

    if (startNodes.empty() || endNodes.empty()) {
        result.resultText = "Error: No reachable intersection within R";
        return result;
    }

    int meetingNode = -1;

    // Dijkstra's algorithm
    while (!pqForward.empty() && !pqBackward.empty()) {
        int currNodeForward = pqForward.top().second;
        int currNodeBackward = pqBackward.top().second;
        {
            auto [currTime, currNode] = pqForward.top();
            pqForward.pop();
            if (visitedStart[currNode]) continue;
            visitedStart[currNode] = true;

            // Check if this node has been visited by backward search
            if (visitedEnd[currNode]) {
                if (double totalTime = timeForward[currNode] + timeBackward[currNode]; totalTime < result.travelTime) {
                    meetingNode = currNode;
                    result.travelTime = totalTime;
                }
            }
            // Check all neighbors
            for (const auto& [neighbor, edge] : adjacencyList[currNode]) {
                // Skip already visited nodes
                double newTime = timeForward[currNode] + (edge.distance/edge.speed)*60;

                if (visitedEnd[neighbor] && newTime + timeBackward[neighbor] < result.travelTime) {
                    meetingNode = neighbor;
                    result.travelTime = newTime + timeBackward[neighbor];
                }

                // Relaxation step
                if (newTime < timeForward[neighbor]) {
                    timeForward[neighbor] = newTime;
                    distForward[neighbor] = distForward[currNode] + edge.distance;
                    prevForward[neighbor] = currNode;
                    pqForward.emplace(newTime, neighbor);
                }
            }

        }
        {
            auto [currTime, currNode] = pqBackward.top();
            pqBackward.pop();

            if (visitedEnd[currNode]) continue;
            visitedEnd[currNode] = true;

            // Check if this node has been visited by forward search
            if (visitedStart[currNode]) {
                if (double totalTime = timeForward[currNode] + timeBackward[currNode]; totalTime < result.travelTime) {
                    meetingNode = currNode;
                    result.travelTime = totalTime;
                }
            }

            // Check all neighbors
            for (const auto& [neighbor, edge] : adjacencyList[currNode]) {
                // Skip already visited nodes
                double newTime = timeBackward[currNode] + (edge.distance/edge.speed)*60;

                if (visitedStart[neighbor] && newTime + timeForward[neighbor] < result.travelTime) {
                    meetingNode = neighbor;
                    result.travelTime = newTime + timeForward[neighbor];
                }

                // Relaxation step
                if (newTime < timeBackward[neighbor]) {
                    timeBackward[neighbor] = newTime;
                    distBackward[neighbor] = distBackward[currNode] + edge.distance;
                    prevBackward[neighbor] = currNode;
                    pqBackward.emplace(newTime , neighbor);
                }
            }
        }
        // Add a more efficient termination condition
        if (timeForward[currNodeForward] + timeBackward[currNodeBackward] >= result.travelTime) break;
    }

    if (meetingNode == -1) {
        result.resultText = "Error: No valid path found";
        return result;
    }

    // Reconstruct forward paths
    std::vector<int> forwardPath;
    for (int at = meetingNode; at != -1; at = prevForward[at]) {
        forwardPath.push_back(at);
    }
    std::reverse(forwardPath.begin(), forwardPath.end());

    // Reconstruct the backward path
    std::vector<int> backwardPath;
    for (int at = prevBackward[meetingNode]; at != -1; at = prevBackward[at]) {
        backwardPath.push_back(at);
    }

    result.path = forwardPath;
    result.path.insert(result.path.end(), backwardPath.begin(), backwardPath.end());
    lastPath = result.path;

    // Calculate walking distance
    result.walkingDistance = distForward[result.path[0]] + distBackward[result.path[result.path.size()-1]];

    // Calculate total distance using Dijkstra's results
    result.totalDistance = distForward[meetingNode] + distBackward[meetingNode];

    // Calculate vehicle distance
    result.vehicleDistance = round((result.totalDistance - result.walkingDistance)*100)/100;

    // Format result string
    std::stringstream ss;
    for (size_t i = 0; i < result.path.size(); i++) {
        ss << result.path[i];
        if (i < result.path.size() - 1) {
            ss << " ";
        }
    }
    ss << std::endl;
    ss << std::fixed << std::setprecision(2) << result.travelTime << " mins" << std::endl;
    ss << std::fixed << std::setprecision(2) << result.totalDistance<< " km" << std::endl;
    ss << std::fixed << std::setprecision(2) << result.walkingDistance << " km" << std::endl;
    ss << std::fixed << std::setprecision(2) << result.vehicleDistance << " km" << std::endl;

    result.resultText = ss.str();

    return result;
}

std::string MapGraph::displayOutput(const std::vector<PathResult> &results) const {

    std::stringstream result;
    std::string line;
    const unsigned long queryNumber = queries.size();

    if (queryNumber == 0) {
        return "No valid query results found in the output file.";
    }
    
    result << "\n===== SUMMARY STATISTICS =====\n";
    result << "Total queries processed: " << queryNumber << std::endl << std::endl;
    for (unsigned long i = 0; i < queryNumber; i++) {
        result << "-----------------\n";
        result << "Query #" << i+1 << ":\n" << results[i].resultText << std::endl;
    }
    
    return result.str();
}

inline std::vector<std::pair<int, double>> MapGraph::findNodesWithinRadius(const double x, const double y, const double R,
    priorityQueue& pq, std::vector<double>& time, std::vector<double>& dist) const {
    std::vector<std::pair<int, double>> result;
    for (int i = 0; i < nodePositions.size(); ++i) {
        if (double distance = calculateDistance(x, y, nodePositions[i].first, nodePositions[i].second); distance <= R) {
            time[i] = (distance / 5.0) * 60.0;
            dist[i] = distance;
            pq.emplace(time[i], i);
            result.emplace_back(i, distance);
        }
    }
    return result;
}

double MapGraph::calculateDistance(const double x1, const double y1, const double x2, const double y2) {
    return std::sqrt(std::pow(x2 - x1, 2) + std::pow(y2 - y1, 2));
}

std::vector<PathResult> MapGraph::runAllQueries() {
    std::vector<PathResult> results;
    
    if (queries.empty()) {
        return results;
    }
    
    try {
        results.reserve(queries.size()); // Preallocate to avoid reallocations
        
        for (auto [startX, startY, endX, endY, R] : queries) {
            PathResult result;
            try {
                // Compute the actual path by finding the shortest path
                result = findShortestPath(startX, startY, endX, endY, R);
            } catch ([[maybe_unused]] const std::exception& e) {
                result.resultText = "Error: Exception during path finding";
            }
            results.push_back(result);
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception during running all queries: " << e.what() << std::endl;
    }
    
    return results;
}
