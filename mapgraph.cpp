#include "mapgraph.h"
#include "qminmax.h"
#include <iomanip>
#include <chrono>
#include <queue>
#include <algorithm>
#include <fstream>
#include <iostream>

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
            nodePositions.push_back({node.x, node.y});
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
    // Find closest nodes to start and end coordinates
    std::pair<int,double> closestToStart = findClosestNode(startX, startY);
    std::pair<int,double> closestToEnd = findClosestNode(endX, endY);
    int startNode = closestToStart.first;
    int endNode = closestToEnd.first;

    std::cerr << "Start Node: " << startNode << std::endl << "End Node: " << endNode << std::endl;

    double startWalking = closestToStart.second;
    double endWalking = closestToEnd.second;

    PathResult result;
    
    std::cerr << "Finding shortest path from node " << startNode << " to node " << endNode << std::endl;
    
    if (startNode == -1 || endNode == -1) {
        std::cerr << "Error: Could not find valid start or end node" << std::endl;
        result.path.clear();
        result.travelTime = 0.0;
        result.walkingDistance = 0.0;
        result.vehicleDistance = 0.0;
        result.resultText = "Error: Could not find valid start or end node";
        return result;
    }
    
    // Print coordinates and found nodes for debugging
    std::cerr << "Query: (" << startX << "," << startY << ") to (" << endX << "," << endY 
              << "), R =" << R
              << " -> nodes: " << startNode << " to " << endNode << std::endl;
    
    // If start and end are the same, return a simple path
    if (startNode == endNode) {
        result.path = {startNode};
        result.travelTime = 0.0;
        result.walkingDistance = 0.0;
        result.vehicleDistance = 0.0;
        
        std::stringstream ss;
        ss << startNode << std::endl;
        ss << std::fixed << std::setprecision(2) << result.travelTime << " mins" << std::endl;
        ss << std::fixed << std::setprecision(2) << result.vehicleDistance + result.walkingDistance << " km" << std::endl;
        ss << std::fixed << std::setprecision(2) << result.walkingDistance << " km" << std::endl;
        ss << std::fixed << std::setprecision(2) << result.vehicleDistance << " km" << std::endl;
        
        result.resultText = ss.str();
        return result;
    }
    
    // Check if nodes are valid indices
    if (startNode >= static_cast<int>(nodePositions.size()) || endNode >= static_cast<int>(nodePositions.size()) ||
        startNode < 0 || endNode < 0 || startWalking > R || endWalking > R) {
        std::cerr << "Error: Invalid node indices: " << startNode << ", " << endNode << std::endl;
        result.path.clear();
        result.travelTime = 0.0;
        result.vehicleDistance = 0.0;
        result.walkingDistance = 0.0;
        result.resultText = "Error: Invalid node indices";
        return result;
    }
    
    // Make sure adjacency list is properly sized
    if (adjacencyList.size() <= static_cast<size_t>(std::max(startNode, endNode))) {
        std::cerr << "Error: Adjacency list not properly sized for nodes" << std::endl;
        std::cerr << "Adjacency list size: " << adjacencyList.size() << ", needed: " << std::max(startNode, endNode) + 1 << std::endl;
        result.path.clear();
        result.travelTime = 0.0;
        result.vehicleDistance = 0.0;
        result.walkingDistance = 0.0;
        result.resultText = "Error: Adjacency list not properly sized";
        return result;
    }
    
    // Check if the nodes have any neighbors
    if (adjacencyList[startNode].empty()) {
        std::cerr << "Error: Start node " << startNode << " has no neighbors" << std::endl;
        result.path.clear();
        result.travelTime = 0.0;
        result.vehicleDistance = 0.0;
        result.walkingDistance = 0.0;
        result.resultText = "Error: Start node has no neighbors";
        return result;
    }
    
    if (adjacencyList[endNode].empty()) {
        std::cerr << "Error: End node " << endNode << " has no neighbors" << std::endl;
        result.path.clear();
        result.travelTime = 0.0;
        result.vehicleDistance = 0.0;
        result.walkingDistance = 0.0;
        result.resultText = "Error: End node has no neighbors";
        return result;
    }

    // Initialize arrays for Dijkstra's algorithm
    std::vector<double> time(nodePositions.size(), std::numeric_limits<double>::infinity());
    std::vector<double> dist(nodePositions.size());
    std::vector<int> prev(nodePositions.size(), -1);
    std::vector<bool> visited(nodePositions.size(), false);
    
    // Priority queue for Dijkstra's algorithm - (distance, node)
    std::priority_queue<std::pair<double, int>, std::vector<std::pair<double, int>>, std::greater<std::pair<double, int>>> pq;
    
    // Start node has time 0
    time[startNode] = 0.0;
    dist[startNode] = 0.0;
    pq.push({0.0, startNode});
    
    // For debugging
    bool endNodeReached = false;
    int nodesExplored = 0;
    
    // Dijkstra's algorithm
    while (!pq.empty()) {
        auto [currTime, currNode] = pq.top();
        pq.pop();
        
        nodesExplored++;
        
        // Skip if we've already processed this node
        if (visited[currNode]) {
            continue;
        }
        
        // Mark as visited
        visited[currNode] = true;
        
        // If we've reached the end node, we're done
        if (currNode == endNode) {
            endNodeReached = true;
            std::cerr << "End node " << endNode << " reached after exploring " << nodesExplored << " nodes" << std::endl;
            break;
        }
        
        // Ensure currNode is valid index for adjacencyList
        if (currNode < 0 || currNode >= static_cast<int>(adjacencyList.size())) {
            std::cerr << "Error: Invalid node index " << currNode << " for adjacency list" << std::endl;
            continue;
        }
        
        // Check all neighbors
        for (const auto& [neighbor, edge] : adjacencyList[currNode]) {
            // Skip already visited nodes
            if (visited[neighbor]) continue;

            double distance = edge.distance;
            double speed = edge.speed;
            double travelTime = (distance/speed)*60;

            // Relaxation step
            double newTime = time[currNode] + travelTime;
            double newDist = dist[currNode] + distance;
            if (newTime < time[neighbor]) {
                time[neighbor] = newTime;
                dist[neighbor] = newDist;
                prev[neighbor] = currNode;
                double heuristic = (calculateDistance(nodePositions[neighbor].first, nodePositions[neighbor].second,
                                                      nodePositions[endNode].first, nodePositions[endNode].second)/max_speed)*60;
                pq.push({newTime + heuristic, neighbor});
            }
        }
    }
    
    // If end node was not reached
    if (!endNodeReached || time[endNode] == std::numeric_limits<double>::infinity()) {
        std::cerr << "Error: No path found from node " << startNode << " to " << endNode << std::endl;
        result.path.clear();
        result.travelTime = 0.0;
        result.vehicleDistance = 0.0;
        result.walkingDistance = 0.0;
        result.resultText = "No path found";
        return result;
    }
    
    // Reconstruct path
    std::vector<int> path;
    for (int at = endNode; at != -1; at = prev[at]) {
        path.push_back(at);
        
        // Safety check to avoid infinite loops
        if (path.size() > nodePositions.size()) {
            std::cerr << "Warning: Path reconstruction loop detected" << std::endl;
            break;
        }
    }
    
    // Reverse the path to go from start to end
    std::reverse(path.begin(), path.end());
    
    // Check if path is valid
    if (path.empty() || path.front() != startNode || path.back() != endNode) {
        std::cerr << "Error: Path reconstruction failed" << std::endl;
        if (!path.empty()) {
            std::cerr << "  Path starts at: " << path.front() << " (should be " << startNode << ")" << std::endl;
            std::cerr << "  Path ends at: " << path.back() << " (should be " << endNode << ")" << std::endl;
        }
        
        // Try direct path as fallback
        result.path = {startNode, endNode};
    } else {
        lastPath = path;
        result.path = path;
    }

    // Calculate walking distance
    result.walkingDistance = startWalking + endWalking;

    // Calculate travel time (already computed during Dijkstra + time of walking distance)
    result.travelTime = time[endNode] + (result.walkingDistance/5)*60;

    // Calculate vehicle path distance (already computed during Dijkstra)
    result.vehicleDistance = dist[endNode];

    // for (size_t i = 0; i < lastPath.size() - 1; i++) {
    //     int u = lastPath[i];
    //     int v = lastPath[i + 1];

    //     std::string edgeKey = std::to_string(u) + "_" + std::to_string(v);

    //     // Check if edge exists in our map
    //     auto it = edgeMap.find(edgeKey);
    //     if (it != edgeMap.end()) {
    //         // Direct lookup into edges array using the index from the map
    //         lastDistance += edges[it->second].distance;
    //     } else {
    //         // Fallback to linear search if not found in map (should not happen)
    //         bool found = false;
    //         for (const auto& edge : edges) {
    //             if ((edge.source == u && edge.destination == v) ||
    //                 (edge.source == v && edge.destination == u)) {
    //                 lastDistance += edge.distance;
    //                 found = true;
    //                 break;
    //             }
    //         }

    //         if (!found) {
    //             // If edge not found at all, use direct distance
    //             if (nodePositions.find(u) != nodePositions.end() &&
    //                 nodePositions.find(v) != nodePositions.end()) {
    //                 double directDist = calculateDistance(
    //                     nodePositions[u].first, nodePositions[u].second,
    //                     nodePositions[v].first, nodePositions[v].second);
    //                 lastDistance += directDist;
    //                 std::cerr << "Warning: Using direct distance for edge " << u << "->" << v << ": " << directDist << " km" << std::endl;
    //             } else {
    //                 std::cerr << "Error: Edge not found between nodes " << u << " and " << v << std::endl;
    //             }
    //         }
    //     }
    // }
    
    
    //Debug output
    std::cerr << "Path found with " << result.path.size() << " nodes:" << std::endl;
    for (size_t i = 0; i < result.path.size(); i++) {
        std::cerr << result.path[i];
        if (i < result.path.size() - 1) {
            std::cerr << " -> ";
        }
    }
    std::cerr << std::endl;
    std::cerr << "Travel time: " << result.travelTime << " mins" << std::endl;
    std::cerr << "Total Distance: " << result.vehicleDistance + result.walkingDistance << " km" << std::endl;
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
    ss << std::fixed << std::setprecision(2) << result.vehicleDistance + result.walkingDistance << " km" << std::endl;
    ss << std::fixed << std::setprecision(2) << result.walkingDistance << " km" << std::endl;
    ss << std::fixed << std::setprecision(2) << result.vehicleDistance << " km" << std::endl;
    
    result.resultText = ss.str();

    return result;
}

std::string MapGraph::compareWithOutput(const std::string& outputFilename) {
    std::ifstream file(outputFilename);
    if (!file.is_open()) {
        return "Error opening output file: " + outputFilename;
    }
    
    std::stringstream result;
    std::string line;
    int queryNumber = 0;
    bool hasMoreQueries = true;
    
    // Accuracy metrics
    int totalPathMatches = 0;
    double totalTimeError = 0.0;
    double totalDistanceError = 0.0;
    double totalWalkingDistanceError = 0.0;
    double totalVehicleDistanceError = 0.0;
    double totalExecTime = 0.0;
    
    // Make sure we have queries loaded
    if (queries.empty()) {
        return "Error: No queries loaded. Please load queries first.";
    }
    
    while (hasMoreQueries && queryNumber < static_cast<int>(queries.size())) {
        std::vector<int> expectedPath;
        double expectedTime = 0.0;
        double expectedTotalDistance = 0.0;
        double expectedWalkingDistance = 0.0;
        double expectedVehicleDistance = 0.0;

        // Read the expected path
        if (std::getline(file, line)) {
            // Check if this is a blank line separating query results
            if (line.empty()) {
                continue;
            }
            
            std::istringstream iss(line);
            int nodeId;
            while (iss >> nodeId) {
                expectedPath.push_back(nodeId);
            }
        } else {
            hasMoreQueries = false;
            break;
        }
        
        // Read expected travel time
        if (std::getline(file, line)) {
            // Extract number from "X.XX mins" format
            size_t pos = line.find(" mins");
            if (pos != std::string::npos) {
                expectedTime = std::stod(line.substr(0, pos));
            }
        } else {
            hasMoreQueries = false;
            break;
        }
        
        // Read expected total distance
        if (std::getline(file, line)) {
            // Extract number from "X.XX km" format
            size_t pos = line.find(" km");
            if (pos != std::string::npos) {
                expectedTotalDistance = std::stod(line.substr(0, pos));
            }
        } else {
            hasMoreQueries = false;
            break;
        }
        
        // Read expected walking distance
        if (std::getline(file, line)) {
            // Extract number from "X.XX km" format
            size_t pos = line.find(" km");
            if (pos != std::string::npos) {
                expectedWalkingDistance = std::stod(line.substr(0, pos));
            }
        } else {
            hasMoreQueries = false;
            break;
        }

        // Read expected vehicle distance
        if (std::getline(file, line)) {
            // Extract number from "X.XX km" format
            size_t pos = line.find(" km");
            if (pos != std::string::npos) {
                expectedVehicleDistance = std::stod(line.substr(0, pos));
            }
        } else {
            hasMoreQueries = false;
            break;
        }
        
        queryNumber++;
        
        // Get the current query
        const Query& currentQuery = queries[queryNumber - 1];
        
        std::cerr << "\n=== Processing comparison for Query #" << queryNumber << " ===" << std::endl;
        
        // Compute the actual path for this specific query
        auto startTime = std::chrono::high_resolution_clock::now();
        PathResult pathResult = findShortestPath(currentQuery.startX, currentQuery.startY,
                                                 currentQuery.endX, currentQuery.endY,
                                                 currentQuery.R);
        auto endTime = std::chrono::high_resolution_clock::now();
        double execTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() / 1000.0;
        totalExecTime += execTime;
        
        // Compare results
        result << "Query #" << queryNumber << ":\n";
        
        // Compare paths
        bool pathMatch = (expectedPath == pathResult.path);
        if (pathMatch) {
            totalPathMatches++;
        }
        result << "  Path match: " << (pathMatch ? "Yes" : "No") << std::endl;
        
        if (!pathMatch) {
            result << "  Expected path: ";
            for (const auto& node : expectedPath) {
                result << node << " ";
            }
            result << std::endl;
            
            result << "  Actual path: ";
            for (const auto& node : pathResult.path) {
                result << node << " ";
            }
            result << std::endl;
        }
        
        // Compare times
        double timeError = std::abs(expectedTime - pathResult.travelTime);
        totalTimeError += timeError;
        result << "  Time error: " << std::fixed << std::setprecision(2) << timeError << " mins";
        if (timeError > 0.01) {
            result << " (Expected: " << expectedTime << ", Actual: " << pathResult.travelTime << ")";
        }
        result << std::endl;
        
        // Compare distances
        double distanceError = std::abs(expectedTotalDistance - (pathResult.walkingDistance + pathResult.vehicleDistance));
        totalDistanceError += distanceError;
        result << "  Distance error: " << std::fixed << std::setprecision(2) << distanceError << " km";
        if (distanceError > 0.01) {
            result << " (Expected: " << expectedTotalDistance << ", Actual: " << (pathResult.walkingDistance + pathResult.vehicleDistance) << ")";
        }
        result << std::endl;
        
        // Compare walking distances
        double walkingDistanceError = std::abs(expectedWalkingDistance - pathResult.walkingDistance);
        totalWalkingDistanceError += walkingDistanceError;
        result << "  Direct distance error: " << std::fixed << std::setprecision(2) << walkingDistanceError << " km";
        if (walkingDistanceError > 0.01) {
            result << " (Expected: " << expectedWalkingDistance << ", Actual: " << pathResult.walkingDistance << ")";
        }
        result << std::endl;

        // Compare vehicle distances
        double vehicleDistanceError = std::abs(expectedVehicleDistance - pathResult.vehicleDistance);
        totalVehicleDistanceError += vehicleDistanceError;
        result << "  Direct distance error: " << std::fixed << std::setprecision(2) << vehicleDistanceError << " km";
        if (vehicleDistanceError > 0.01) {
            result << " (Expected: " << expectedVehicleDistance << ", Actual: " << pathResult.vehicleDistance << ")";
        }
        result << std::endl;
        
        // Execution time for this query
        result << "  Execution time: " << std::fixed << std::setprecision(3) << execTime << " seconds" << std::endl;
        
        // Read blank line separating queries (if any)
        std::getline(file, line);
    }
    
    file.close();
    
    if (queryNumber == 0) {
        return "No valid query results found in the output file.";
    }
    
    // Add summary metrics
    double pathMatchAccuracy = (queryNumber > 0) ? (static_cast<double>(totalPathMatches) / queryNumber) * 100.0 : 0.0;
    double avgTimeError = (queryNumber > 0) ? totalTimeError / queryNumber : 0.0;
    double avgDistanceError = (queryNumber > 0) ? totalDistanceError / queryNumber : 0.0;
    double avgWalkingDistanceError = (queryNumber > 0) ? totalWalkingDistanceError / queryNumber : 0.0;
    double avgVehicleDistanceError = (queryNumber > 0) ? totalVehicleDistanceError / queryNumber : 0.0;
    
    result << "\n===== SUMMARY STATISTICS =====\n";
    result << "Total queries processed: " << queryNumber << std::endl;
    result << "Path match accuracy: " << std::fixed << std::setprecision(2) << pathMatchAccuracy 
           << "% (" << totalPathMatches << "/" << queryNumber << " correct)" << std::endl;
    result << "Average time error: " << std::fixed << std::setprecision(2) << avgTimeError << " mins" << std::endl;
    result << "Average total distance error: " << std::fixed << std::setprecision(2) << avgDistanceError << " km" << std::endl;
    result << "Average walking distance error: " << std::fixed << std::setprecision(2) << avgWalkingDistanceError << " km" << std::endl;
    result << "Average vehicle distance error: " << std::fixed << std::setprecision(2) << avgVehicleDistanceError << " km" << std::endl;
    result << "Total execution time: " << std::fixed << std::setprecision(3) << totalExecTime << " seconds" << std::endl;
    
    return result.str();
}

std::pair<int,double> MapGraph::findClosestNode(double x, double y) const {
    if (nodePositions.empty()) {
        return {-1,std::numeric_limits<double>::infinity()};
    }
    
    int closestNode = -1;
    double minDistance = std::numeric_limits<double>::infinity();
    
    // Use direct access to node positions from the spatial index
    for (unsigned long long nodeId = 0; nodeId < nodePositions.size(); nodeId++) {
        double distance = calculateDistance(x, y, nodePositions[nodeId].first, nodePositions[nodeId].second);
        if (distance < minDistance) {
            minDistance = distance;
            closestNode = nodeId;
        }
    }
    
    return {closestNode, minDistance};
}

double MapGraph::calculateDistance(double x1, double y1, double x2, double y2) const {
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
                std::cerr << "\n\n=== Processing Query #" << (i+1) << " ===" << std::endl;
                std::cerr << "Coordinates: (" << query.startX << ", " << query.startY 
                          << ") to (" << query.endX << ", " << query.endY 
                          << "), maxSpeed: " << query.R << std::endl;
                
                // Compute the actual path by finding the shortest path
                result = findShortestPath(query.startX, query.startY, query.endX, query.endY, query.R);
                
                // After findShortestPath has run, the lastPath, lastTravelTime etc. will be populated
                // result.path = lastPath;
                // result.travelTime = lastTravelTime;
                // result.distance = lastDistance;
                // result.directDistance = lastDirectDistance;
                
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
