#pragma once

#include <vector>

// Holds all distances and the total individual distance.
struct DijkstraResult {
	double totalDistance;
	bool reachable;
	std::vector<int> path;
	std::vector<double> allDistances;
	double elapsedMs = 0.0;
};