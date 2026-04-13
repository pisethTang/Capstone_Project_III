#pragma once

#include <functional>

#include "../types/vec3.hpp"

struct ObjNormalizeTransform {
	Vec3 center{0, 0, 0};
	double scale = 1.0;
};

struct TorusParams {
	Vec3 center{0, 0, 0};
	double majorRadius = 1.0;  // R
	double minorRadius = 0.25; // r
};

struct SaddleParams {
	Vec3 center{0, 0, 0};
	double a = 0.5; // z = a(x^2 - y^2)
};

struct ParamSurface {
	std::function<Vec3(double, double)> eval;
};
