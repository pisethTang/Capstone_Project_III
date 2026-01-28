#include <iostream>
#include <string>

#include "analytics.hpp"
#include "mesh_engine.hpp"

int main(int argc, char *argv[]) {
	// accept cmd line arguments

	if (argc < 4) {
		std::cerr << "Usage: ./engine/bin/main <start_id> <end_id> <model_path> [mode]"
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
	// int endVertexIndex = 11
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