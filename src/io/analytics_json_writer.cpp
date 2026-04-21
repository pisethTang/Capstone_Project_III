#include "../../include/geodesic_lab/io/analytics_json_writer.hpp"

#include <fstream>
#include <iostream>

#include "../../include/geodesic_lab/io/json_escape.hpp"

void writeAnalyticsJSON(const std::string &outputFilename,
                        const std::string &outputPath,
                        const AnalyticsResult &res) {
	const std::string fullPath = outputPath + outputFilename;
	std::ofstream file(fullPath);
	if (!file.is_open()) {
		std::cerr << "Error: Could not write " << fullPath << std::endl;
		return;
	}

	file << "{\n";
	file << "  \"inputFileName\": \"" << jsonEscape(res.inputFileName)
	     << "\",\n";
	file << "  \"startId\": " << res.startId << ",\n";
	file << "  \"endId\": " << res.endId << ",\n";
	file << "  \"surfaceType\": \"" << jsonEscape(res.surfaceType)
	     << "\",\n";
	file << "  \"surfaceParams\": "
	     << (res.surfaceParams.empty() ? "null" : res.surfaceParams)
	     << ",\n";
	file << "  \"elapsedMs\": " << res.elapsedMs << ",\n";
	file << "  \"error\": \"" << jsonEscape(res.error) << "\",\n";
	file << "  \"curves\": [\n";

	for (size_t ci = 0; ci < res.curves.size(); ci++) {
		const auto &c = res.curves[ci];
		file << "    {\n";
		file << "      \"name\": \"" << jsonEscape(c.name) << "\",\n";
		file << "      \"length\": " << c.length << ",\n";
		file << "      \"points\": [";
		for (size_t pi = 0; pi < c.points.size(); pi++) {
			const auto &p = c.points[pi];
			file << "[" << p.x << ", " << p.y << ", " << p.z << "]";
			if (pi + 1 < c.points.size())
				file << ", ";
		}
		file << "]\n";
		file << "    }";
		file << (ci + 1 < res.curves.size() ? ",\n" : "\n");
	}

	file << "  ]\n";
	file << "}\n";
}
