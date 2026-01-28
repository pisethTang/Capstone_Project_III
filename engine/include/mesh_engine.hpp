#pragma once

#include <string>
#include <vector>

#include "common.hpp"

struct Edge {
	int targetVertex;
	double weight;
};

// Holds all distances and the total individual distance
struct DijkstraResult {
	double totalDistance;
	bool reachable;
	std::vector<int> path;
	std::vector<double> allDistances;
};

class MeshEngine {
  public:
	std::vector<Vec3> vertices;
	std::vector<std::vector<Edge>> graph;
	std::vector<Face> faces;

	bool loadOBJ(const std::string &filename);
	void addEdge(int v1_idx, int v2_idx);
	DijkstraResult solve(int start, int target);
	void writeJSON(const std::string &outputFilename,
	               const std::string &outputPath,
	               const std::string &inputFileName,
	               const std::vector<double> &all_dists,
	               const DijkstraResult &res);
};
