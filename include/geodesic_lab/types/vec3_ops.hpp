#pragma once

#include <algorithm>
#include <cmath>

#include "vec3.hpp"

inline Vec3 vadd(const Vec3 &a, const Vec3 &b) {
	return {a.x + b.x, a.y + b.y, a.z + b.z};
}

inline Vec3 vsub(const Vec3 &a, const Vec3 &b) {
	return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline Vec3 vmul(const Vec3 &a, double s) {
	return {a.x * s, a.y * s, a.z * s};
}

inline double vdot(const Vec3 &a, const Vec3 &b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline double vlen(const Vec3 &a) {
	return std::sqrt(vdot(a, a));
}

inline Vec3 vnormalize(const Vec3 &a) {
	const double l = vlen(a);
	if (l <= 1e-12)
		return {0, 0, 0};
	return {a.x / l, a.y / l, a.z / l};
}

inline Vec3 vcross(const Vec3 &a, const Vec3 &b) {
	return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
	        a.x * b.y - a.y * b.x};
}

inline Vec3 vlerp(const Vec3 &a, const Vec3 &b, double t) {
	return vadd(vmul(a, 1.0 - t), vmul(b, t));
}

inline double clampd(double v, double lo, double hi) {
	return std::max(lo, std::min(hi, v));
}

inline double vdist(const Vec3 &a, const Vec3 &b) {
	return vlen(vsub(a, b));
}
