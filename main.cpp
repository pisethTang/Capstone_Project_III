#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

#include "common.hpp"

// Data modeling (will be moved into their own headers soon ... )

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

// Analytics module is header-style and relies on common.hpp.
#include "analytics.hpp"

double dist(const Vec3 &v1, const Vec3 &v2) {
	return std::sqrt(std::pow(v2.x - v1.x, 2) + std::pow(v2.y - v1.y, 2) +
	                 std::pow(v2.z - v1.z, 2));
}

// Main class
class MeshEngine {
  public:
	std::vector<Vec3> vertices;
	std::vector<std::vector<Edge>> graph;
	std::vector<Face> faces;

	bool loadOBJ(const std::string &filename) {
		std::ifstream file(filename);
		if (!file.is_open())
			return false;
		vertices.clear();
		graph.clear();
		faces.clear();

		std::string line;
		while (std::getline(file, line)) {
			std::istringstream iss(line);
			std::string type;
			iss >> type;
			if (type == "v") {
				Vec3 v;
				iss >> v.x >> v.y >> v.z;
				vertices.push_back(v);
			} else if (type == "f") {
				std::vector<std::string> tokens;
				std::string token;
				while (iss >> token) {
					tokens.push_back(token);
				}
				if (tokens.size() < 3)
					continue;

				auto toIndex = [&](const std::string &t) -> int {
					const auto slash = t.find('/');
					const std::string head =
					    (slash == std::string::npos) ? t : t.substr(0, slash);
					int idx = 0;
					try {
						idx = std::stoi(head);
					} catch (...) {
						return -1;
					}
					if (idx == 0)
						return -1;
					int resolved =
					    (idx > 0) ? (idx - 1)
					              : (static_cast<int>(vertices.size()) + idx);
					return resolved;
				};

				std::vector<int> faceIndices;
				faceIndices.reserve(tokens.size());
				for (const auto &t : tokens) {
					int resolved = toIndex(t);
					if (resolved < 0 || resolved >= (int)vertices.size()) {
						faceIndices.clear();
						break;
					}
					faceIndices.push_back(resolved);
				}
				if (faceIndices.size() < 3)
					continue;

				for (size_t i = 1; i + 1 < faceIndices.size(); i++) {
					const int a = faceIndices[0];
					const int b = faceIndices[i];
					const int c = faceIndices[i + 1];
					faces.push_back({a, b, c});
					addEdge(a, b);
					addEdge(b, c);
					addEdge(c, a);
				}
			}
		}
		return true;
	}

	void addEdge(int v1_idx, int v2_idx) {
		const int maxIndex = std::max(v1_idx, v2_idx);
		if (maxIndex >= 0 && graph.size() <= static_cast<size_t>(maxIndex)) {
			graph.resize(static_cast<size_t>(maxIndex) + 1);
		}
		double d = dist(vertices[v1_idx], vertices[v2_idx]);
		graph[v1_idx].push_back({v2_idx, d});
		graph[v2_idx].push_back({v1_idx, d});
	}

	DijkstraResult solve(int start, int target) {
		int n = vertices.size();
		std::vector<double> min_dist(n, std::numeric_limits<double>::max());
		std::vector<int> parent(n, -1);

		min_dist[start] = 0;
		std::priority_queue<std::pair<double, int>,
		                    std::vector<std::pair<double, int>>, std::greater<>>
		    pq;
		pq.push({0, start});

		while (!pq.empty()) {
			double d = pq.top().first;
			int u = pq.top().second;
			pq.pop();

			if (u == target)
				break;
			if (d > min_dist[u])
				continue;

			for (auto &edge : graph[u]) {
				if (min_dist[u] + edge.weight < min_dist[edge.targetVertex]) {
					min_dist[edge.targetVertex] = min_dist[u] + edge.weight;
					parent[edge.targetVertex] = u;
					pq.push({min_dist[edge.targetVertex], edge.targetVertex});
				}
			}
		}

		std::vector<int> path;
		const bool reachable = (start == target) || (parent[target] != -1);
		if (reachable) {
			for (int v = target; v != -1; v = parent[v]) {
				path.push_back(v);
			}
			std::reverse(path.begin(), path.end());
		}

		return {min_dist[target], reachable, path, min_dist};
	}

	// New helper to save results for our React Frontend
	void writeJSON(const std::string &outputFilename,
	               const std::string &outputPath,
	               const std::string &inputFileName,
	               const std::vector<double> &all_dists,
	               const DijkstraResult &res) {
		std::string full_path = outputPath + outputFilename;
		std::ofstream file(full_path);
		if (!file.is_open()) {
			std::cerr << "Error: Could not write " << full_path << std::endl;
			return;
		}
		file << "{\n";
		file << "  \"inputFileName\": \"" << jsonEscape(inputFileName)
		     << "\",\n";
		file << "  \"reachable\": " << (res.reachable ? "true" : "false")
		     << ",\n";
		file << "  \"totalDistance\": ";
		if (!res.reachable || !std::isfinite(res.totalDistance) ||
		    res.totalDistance >= std::numeric_limits<double>::max() / 2) {
			file << "null";
		} else {
			file << res.totalDistance;
		}
		file << ",\n";
		file << "  \"path\": [";
		for (size_t i = 0; i < res.path.size(); ++i) {
			file << res.path[i] << (i == res.path.size() - 1 ? "" : ", ");
		}
		file << "],\n";
		file << "  \"allDistances\": [";
		for (size_t i = 0; i < all_dists.size(); ++i) {
			file << all_dists[i] << (i == all_dists.size() - 1 ? "" : ", ");
		}
		file << "]\n}";
	}
};

int main(int argc, char *argv[]) {
	// accept cmd line arguments

	if (argc < 4) {
		std::cerr << "Usage: ./main <start_id> <end_id> <model_path> [mode]"
		          << std::endl;
		std::cerr
		    << "  mode: analytics (writes ./frontend/public/analytics.json)"
		    << std::endl;
		std::cerr << "  mode: heat (writes ./frontend/public/heat_result.json)"
		          << std::endl;
		return 1;
	}

	int startVertexIndex = std::stoi(argv[1]);
	int endVertexIndex = std::stoi(argv[2]);
	std::string fileName = argv[3];
	std::string mode = (argc >= 5) ? std::string(argv[4]) : std::string();

	MeshEngine engine;
	// std::string fileName = "./frontend/public/data/icosahedron.obj";

	// We will load the failing dataset
	if (!engine.loadOBJ(fileName)) {
		std::cerr << "Error: Could not find " << fileName << std::endl;
		return 1;
	}

	std::string outputPath = "./frontend/public/";
	std::string inputFileName = fileName;

	if (mode == "analytics") {
		AnalyticsResult analytics = computeAnalyticsForModel(
		    inputFileName, startVertexIndex, endVertexIndex, engine.vertices,
		    engine.faces);
		writeAnalyticsJSON("analytics.json", outputPath, analytics);
		std::cout << "--- Geodesic Lab: Analytics ---" << std::endl;
		if (!analytics.error.empty()) {
			std::cout << "Error: " << analytics.error << std::endl;
		} else {
			std::cout << "Surface: " << analytics.surfaceType << std::endl;
			std::cout << "Curves: " << analytics.curves.size() << std::endl;
		}
		std::cout << "------------------------------" << std::endl;
		return analytics.error.empty() ? 0 : 2;
	}

	if (mode == "heat") {
		AnalyticsResult heat =
		    computeHeatForModel(inputFileName, startVertexIndex, endVertexIndex,
		                        engine.vertices, engine.faces);
		writeAnalyticsJSON("heat_result.json", outputPath, heat);
		std::cout << "--- Geodesic Lab: Heat Method ---" << std::endl;
		if (!heat.error.empty()) {
			std::cout << "Error: " << heat.error << std::endl;
		} else {
			std::cout << "Surface: " << heat.surfaceType << std::endl;
			std::cout << "Curves: " << heat.curves.size() << std::endl;
		}
		std::cout << "--------------------------------" << std::endl;
		return heat.error.empty() ? 0 : 2;
	}

	// int startVertexIndex = 0;
	// int endVertexIndex =
	//     11; // Adjust this based on your specific OBJ vertex count;
	// Start: Vertex 0 (0,0,0) | Target: Vertex 2 (1,1,0)
	DijkstraResult result = engine.solve(startVertexIndex, endVertexIndex);

	std::cout << "--- Geodesic Lab: Dijkstra Test ---" << std::endl;
	if (!result.reachable) {
		std::cout << "Target Distance: (unreachable)" << std::endl;
	} else {
		std::cout << "Target Distance: " << result.totalDistance << std::endl;
	}
	std::cout << "Path: ";
	for (int v : result.path)
		std::cout << v << " ";
	std::cout << "\n-----------------------------------" << std::endl;
	engine.writeJSON("result.json", outputPath, inputFileName,
	                 result.allDistances, result);

	return 0;
}