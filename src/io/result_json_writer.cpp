#include "../../include/geodesic_lab/io/result_json_writer.hpp"

#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>

#include "../../include/geodesic_lab/io/json_escape.hpp"

void writeResultJSON(const std::string &outputFilename,
                     const std::string &outputPath,
                     const std::string &inputFileName,
                     const std::vector<double> &all_dists,
                     const DijkstraResult &res) {
	const std::string full_path = outputPath + outputFilename;
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
