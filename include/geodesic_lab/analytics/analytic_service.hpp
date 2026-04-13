#pragma once

#include <string>
#include <vector>

#include "../types/face.hpp"
#include "../types/vec3.hpp"
#include "analytic_types.hpp"

AnalyticsResult computeAnalyticsForModel(const std::string &inputFileName,
										 int startId, int endId,
										 const std::vector<Vec3> &objVertices,
										 const std::vector<Face> &faces);

AnalyticsResult computeHeatForModel(const std::string &inputFileName,
									int startId, int endId,
									const std::vector<Vec3> &objVertices,
									const std::vector<Face> &faces);
