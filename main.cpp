#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

// Data modeling (will be moved into their own headers soon ... )
struct Vec3 {
	double x, y, z;
};

struct Edge {
	int targetVertex;
	double weight;
};

// Holds all distances and the total individual distance
struct DijkstraResult {
	double totalDistance;
	std::vector<int> path;
	std::vector<double> allDistances;
};

static std::string jsonEscape(const std::string &s) {
	std::string out;
	out.reserve(s.size() + 8);
	for (char c : s) {
		switch (c) {
		case '\\':
			out += "\\\\";
			break;
		case '"':
			out += "\\\"";
			break;
		case '\n':
			out += "\\n";
			break;
		case '\r':
			out += "\\r";
			break;
		case '\t':
			out += "\\t";
			break;
		default:
			out += c;
			break;
		}
	}
	return out;
}

double dist(const Vec3 &v1, const Vec3 &v2) {
	return std::sqrt(std::pow(v2.x - v1.x, 2) + std::pow(v2.y - v1.y, 2) +
	                 std::pow(v2.z - v1.z, 2));
}

// Main class
class MeshEngine {
  public:
	std::vector<Vec3> vertices;
	std::vector<std::vector<Edge>> graph;

	bool loadOBJ(const std::string &filename) {
		std::ifstream file(filename);
		if (!file.is_open())
			return false;

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
				int idx1, idx2, idx3;
				iss >> idx1 >> idx2 >> idx3;
				addEdge(idx1 - 1, idx2 - 1);
				addEdge(idx2 - 1, idx3 - 1);
				addEdge(idx3 - 1, idx1 - 1);
			}
		}
		return true;
	}

	void addEdge(int v1_idx, int v2_idx) {
		if (graph.size() <= std::max(v1_idx, v2_idx)) {
			graph.resize(std::max(v1_idx, v2_idx) + 1);
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
		for (int v = target; v != -1; v = parent[v]) {
			path.push_back(v);
		}
		std::reverse(path.begin(), path.end());

		return {min_dist[target], path, min_dist};
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
		file << "  \"totalDistance\": " << res.totalDistance << ",\n";
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

int main(int argc, char* argv[]) {
	// accept cmd line arguments 

	if (argc < 4){
		std::cerr << "Usage: ./main <start_id> <end_id> <model_path>" << std::endl;
	}

	int startVertexIndex = std::stoi(argv[1]);
	int endVertexIndex = std::stoi(argv[2]);
	std::string fileName = argv[3];

	MeshEngine engine;
	// std::string fileName = "./frontend/public/data/icosahedron.obj";

	// We will load the failing dataset
	if (!engine.loadOBJ(fileName)) {
		std::cerr << "Error: Could not find zig_zag.obj" << std::endl;
		return 1;
	}

	// int startVertexIndex = 0;
	// int endVertexIndex =
	//     11; // Adjust this based on your specific OBJ vertex count;
	// Start: Vertex 0 (0,0,0) | Target: Vertex 2 (1,1,0)
	DijkstraResult result = engine.solve(startVertexIndex, endVertexIndex);

	std::cout << "--- Geodesic Lab: Dijkstra Test ---" << std::endl;
	std::cout << "Target Distance: " << result.totalDistance << std::endl;
	std::cout << "Path: ";
	for (int v : result.path)
		std::cout << v << " ";
	std::cout << "\n-----------------------------------" << std::endl;
	std::string outputPath = "./frontend/public/";
	std::string inputFileName = fileName;

	engine.writeJSON("result.json", outputPath, inputFileName,
	                 result.allDistances, result);

	return 0;
}