#pragma once

#include <vector>

#include "../analytics/analytic_types.hpp"
#include "../types/face.hpp"
#include "../types/vec3.hpp"

AnalyticsCurve makeHeatMethodGeodesic(const std::vector<Vec3> &verts,
									  const std::vector<Face> &faces,
									  int startId, int endId);
