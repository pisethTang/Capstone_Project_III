#pragma once

#include <string>
#include <vector>

#include "../algorithms/dijkstra_result.hpp"

void writeResultJSON(const std::string &outputFilename,
					 const std::string &outputPath,
					 const std::string &inputFileName,
					 const std::vector<double> &all_dists,
					 const DijkstraResult &res);
