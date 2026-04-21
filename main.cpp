#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

#include "include/geodesic_lab/algorithms/dijkstra_result.hpp"
#include "include/geodesic_lab/algorithms/dijkstra_solver.hpp"
#include "include/geodesic_lab/analytics/analytic_service.hpp"
#include "include/geodesic_lab/io/analytics_json_writer.hpp"
#include "include/geodesic_lab/io/obj_loader.hpp"
#include "include/geodesic_lab/io/result_json_writer.hpp"
#include "include/geodesic_lab/mesh/adjacency_builder.hpp"
#include "include/geodesic_lab/mesh/mesh.hpp"

// Main class
class MeshEngine {
  public:
	Mesh mesh;

	bool loadOBJ(const std::string &filename) {
		return loadOBJIntoMesh(
		    filename, mesh,
		    [this](int v1_idx, int v2_idx) {
			    addUndirectedEdge(mesh, v1_idx, v2_idx);
		    });
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

	if (!engine.loadOBJ(fileName)) {
		std::cerr << "Error: Could not find " << fileName << std::endl;
		return 1;
	}

	std::string outputPath = "./frontend/public/";
	std::string inputFileName = fileName;


	

	if (mode == "analytics") { // analytics method
		auto t0 = std::chrono::high_resolution_clock::now();
		AnalyticsResult analytics = computeAnalyticsForModel(inputFileName, startVertexIndex,
															endVertexIndex, engine.mesh.vertices,
		    												engine.mesh.faces);
		auto t1 = std::chrono::high_resolution_clock::now();
		analytics.elapsedMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
		writeAnalyticsJSON("analytics.json", outputPath, analytics);
		std::cout << "--- Geodesic Lab: Analytics ---" << std::endl;
		if (!analytics.error.empty()) {
			std::cout << "Error: " << analytics.error << std::endl;
		} else {
			std::cout << "Surface: " << analytics.surfaceType << std::endl;
			std::cout << "Curves: " << analytics.curves.size() << std::endl;
			std::cout << "Elapsed: " << analytics.elapsedMs << " ms" << std::endl;
		}
		std::cout << "------------------------------" << std::endl;
		return analytics.error.empty() ? 0 : 2;
	}
	else if (mode == "heat") { // heat method
		auto t0 = std::chrono::high_resolution_clock::now();
		AnalyticsResult heat = computeHeatForModel(inputFileName, startVertexIndex, endVertexIndex,
		                        						engine.mesh.vertices, engine.mesh.faces);
		auto t1 = std::chrono::high_resolution_clock::now();
		heat.elapsedMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
		writeAnalyticsJSON("heat_result.json", outputPath, heat);
		std::cout << "--- Geodesic Lab: Heat Method ---" << std::endl;
		if (!heat.error.empty()) {
			std::cout << "Error: " << heat.error << std::endl;
		} else {
			std::cout << "Surface: " << heat.surfaceType << std::endl;
			std::cout << "Curves: " << heat.curves.size() << std::endl;
			std::cout << "Elapsed: " << heat.elapsedMs << " ms" << std::endl;
		}
		std::cout << "--------------------------------" << std::endl;
		return heat.error.empty() ? 0 : 2;
	}
	else { // Dijkstra's method
		auto t0 = std::chrono::high_resolution_clock::now();
		DijkstraResult result = solveDijkstra(engine.mesh, startVertexIndex, endVertexIndex);
		auto t1 = std::chrono::high_resolution_clock::now();
		result.elapsedMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

		std::cout << "--- Geodesic Lab: Dijkstra Test ---" << std::endl;
		if (!result.reachable) {
			std::cout << "Target Distance: (unreachable)" << std::endl;
		} else {
			std::cout << "Target Distance: " << result.totalDistance << std::endl;
		}
		std::cout << "Elapsed: " << result.elapsedMs << " ms" << std::endl;
		std::cout << "Path: ";
		for (int v : result.path)
		std::cout << v << " ";
		std::cout << "\n-----------------------------------" << std::endl;
		writeResultJSON("result.json", outputPath, inputFileName,
			result.allDistances, result);
	}



	return 0;
}