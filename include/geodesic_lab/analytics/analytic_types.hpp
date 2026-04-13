#pragma once

#include <string>
#include <vector>

#include "../types/vec3.hpp"

struct AnalyticsCurve {
	std::string name;
	double length = 0.0;
	std::vector<Vec3> points;
};

struct AnalyticsResult {
	std::string inputFileName;
	int startId = -1;
	int endId = -1;
	std::string surfaceType;
	std::vector<AnalyticsCurve> curves;
	std::string error;
};
