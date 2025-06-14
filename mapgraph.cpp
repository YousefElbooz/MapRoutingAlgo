#include "mapgraph.h"
#include "qminmax.h"
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <cmath>

MapGraph::MapGraph() {}

MapGraph::~MapGraph() {}

bool MapGraph::empty() {
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
    
        // Read number of nodes
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
            Node node;
            file >> node.id >> node.x >> node.y;
            
            // Check for invalid node data
            if (file.fail()) {
                std::cerr << "Error reading node data at index " << i << std::endl;
                return false;
            }
            
            // Create spatial index for faster lookups
            nodePositions.emplace_back(node.x, node.y);
        }
        
        // Read number of edges
        int numEdges;
        file >> numEdges;

        if (numEdges <= 0 || numEdges > 10000000) { // Sanity check for edge count
            std::cerr << "Invalid number of edges: " << numEdges << std::endl;
            return false;
        }

        // Reserve capacity
        edges.reserve(numEdges);

        // Initialize adjacency list
        adjacencyList.resize(numNodes);

        max_speed = 0;
        // Read edge information
        for (int i = 0; i < numEdges; i++) {
            int source, destination;
            Edge edge;
            file >> source >> destination >> edge.distance >> edge.speed;

            max_speed = qMax(max_speed, edge.speed);

            // Check for invalid edge data
            if (file.fail()) {
                std::cerr << "Error reading edge data at index " << i << std::endl;
                return false;
            }

            // Validate edge data
            // if (edge.source < 0 || edge.source >= numNodes ||
            //     edge.destination < 0 || edge.destination >= numNodes ||
            //     edge.distance < 0 || edge.speed <= 0) {
            //     std::cerr << "Invalid edge data at index " << i << ": "
            //               << source << " " << destination << " "
            //               << distance << std::endl;
            //     continue; // Skip invalid edge
            // }

            edges.push_back({source,destination});

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
        
        // Read number of queries
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
            Query q;
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
    std::vector<double> timeForward(nodePositions.size(), std::numeric_limits<double>::infinity());
    std::vector<double> timeBackward(nodePositions.size(), std::numeric_limits<double>::infinity());
    std::vector<double> distForward(nodePositions.size(), 0.0);
    std::vector<double> distBackward(nodePositions.size(), 0.0);
    std::vector<int> prevForward(nodePositions.size(), -1);
    std::vector<int> prevBackward(nodePositions.size(), -1);
    std::vector<bool> visitedStart(nodePositions.size(), false);
    std::vector<bool> visitedEnd(nodePositions.size(), false);

    // Find closest nodes to start and end coordinates
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
                double totalTime = timeForward[currNode] + timeBackward[currNode];
                if (totalTime < result.travelTime) {
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
                double totalTime = timeForward[currNode] + timeBackward[currNode];
                if (totalTime < result.travelTime) {
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

    // Reconstruct backward path
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

    // Debug output
    std::cerr << "Path found with " << result.path.size() << " nodes:" << std::endl;
    for (size_t i = 0; i < result.path.size(); i++) {
        std::cerr << result.path[i];
        if (i < result.path.size() - 1) {
            std::cerr << " -> ";
        }
    }
    std::cerr << std::endl;
    std::cerr << "Travel time: " << result.travelTime << " mins" << std::endl;
    std::cerr << "Total Distance: " << result.totalDistance << " km" << std::endl;
    std::cerr << "Walking distance: " << result.walkingDistance << " km" << std::endl;
    std::cerr << "Vehicle Distance: " << result.vehicleDistance << " km" << std::endl;

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
    bool hasMoreQueries = true;

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

inline std::vector<std::pair<int, double>> MapGraph::findNodesWithinRadius(double x, double y, double R,
    priorityQueue& pq, std::vector<double>& time, std::vector<double>& dist) const {
    std::vector<std::pair<int, double>> result;
    for (int i = 0; i < nodePositions.size(); ++i) {
        double distance = calculateDistance(x, y, nodePositions[i].first, nodePositions[i].second);
        if (distance <= R) {
            time[i] = (distance / 5.0) * 60.0;
            dist[i] = distance;
            pq.emplace(time[i], i);
            result.emplace_back(i, distance);
        }
    }
    return result;
}

double MapGraph::calculateDistance(double x1, double y1, double x2, double y2) {
    return std::sqrt(std::pow(x2 - x1, 2) + std::pow(y2 - y1, 2));
}

std::vector<PathResult> MapGraph::runAllQueries() {
    std::vector<PathResult> results;
    
    if (queries.empty()) {
        std::cerr << "No queries loaded." << std::endl;
        return results;
    }
    
    try {
        results.reserve(queries.size()); // Preallocate to avoid reallocations
        
        for (size_t i = 0; i < queries.size(); i++) {
            const auto& query = queries[i];
            
            PathResult result;
            try {
                // Compute the actual path by finding the shortest path
                result = findShortestPath(query.startX, query.startY, query.endX, query.endY, query.R);

                // Debug output
                std::cerr << "Query #" << (i+1) << " result: " << std::endl;
                std::cerr << "  Path: ";
                for (const auto& node : result.path) {
                    std::cerr << node << " ";
                }
                std::cerr << std::endl;
                std::cerr << "  Travel time: " << result.travelTime << " mins" << std::endl;
                std::cerr << "  Vehicle Distance: " << result.vehicleDistance << " km" << std::endl;
                std::cerr << "  Walking distance: " << result.walkingDistance << " km" << std::endl;
                
            } catch (const std::exception& e) {
                std::cerr << "Exception processing query " << i << ": " << e.what() << std::endl;
                result.resultText = "Error: Exception during path finding";
            }
            results.push_back(result);
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception during running all queries: " << e.what() << std::endl;
    }
    
    return results;
}
