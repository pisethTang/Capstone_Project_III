#pragma once

#include <vector>

#include "analytic_params.hpp"

ObjNormalizeTransform computeNormalizeTransform(const std::vector<Vec3> &verts);

TorusParams estimateTorusParams(const std::vector<Vec3> &verts);

SaddleParams estimateSaddleParams(const std::vector<Vec3> &verts);

Vec3 applyNormalize(const ObjNormalizeTransform &t, const Vec3 &p);