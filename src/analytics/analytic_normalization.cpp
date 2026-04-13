#include "../../include/geodesic_lab/analytics/analytic_normalization.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include "../../include/geodesic_lab/types/vec3_ops.hpp"

ObjNormalizeTransform
computeNormalizeTransform(const std::vector<Vec3> &verts) {
	ObjNormalizeTransform out;
	if (verts.empty())
		return out;

	Vec3 minV{std::numeric_limits<double>::infinity(),
	          std::numeric_limits<double>::infinity(),
	          std::numeric_limits<double>::infinity()};
	Vec3 maxV{-std::numeric_limits<double>::infinity(),
	          -std::numeric_limits<double>::infinity(),
	          -std::numeric_limits<double>::infinity()};

	for (const auto &v : verts) {
		minV.x = std::min(minV.x, v.x);
		minV.y = std::min(minV.y, v.y);
		minV.z = std::min(minV.z, v.z);
		maxV.x = std::max(maxV.x, v.x);
		maxV.y = std::max(maxV.y, v.y);
		maxV.z = std::max(maxV.z, v.z);
	}

	out.center = {(minV.x + maxV.x) * 0.5, (minV.y + maxV.y) * 0.5,
	              (minV.z + maxV.z) * 0.5};
	const double sx = maxV.x - minV.x;
	const double sy = maxV.y - minV.y;
	const double sz = maxV.z - minV.z;
	const double maxSize = std::max({sx, sy, sz});
	out.scale = (maxSize > 1e-12) ? (2.0 / maxSize) : 1.0;
	return out;
}

TorusParams estimateTorusParams(const std::vector<Vec3> &verts) {
	TorusParams out;
	if (verts.empty())
		return out;

	const ObjNormalizeTransform t = computeNormalizeTransform(verts);
	out.center = t.center;

	double sumR = 0.0;
	int countR = 0;
	std::vector<double> rSamples;
	rSamples.reserve(verts.size());
	for (const auto &v : verts) {
		const double dx = v.x - out.center.x;
		const double dy = v.y - out.center.y;
		const double dz = v.z - out.center.z;
		const double rho = std::sqrt(dx * dx + dy * dy);
		if (std::isfinite(rho)) {
			sumR += rho;
			countR++;
		}
		rSamples.push_back(std::sqrt(std::pow(rho, 2) + std::pow(dz, 2)));
	}
	if (countR > 0)
		out.majorRadius = sumR / countR;

	double sumr = 0.0;
	int countr = 0;
	for (const auto &v : verts) {
		const double dx = v.x - out.center.x;
		const double dy = v.y - out.center.y;
		const double dz = v.z - out.center.z;
		const double rho = std::sqrt(dx * dx + dy * dy);
		const double rr = std::sqrt(
		    (rho - out.majorRadius) * (rho - out.majorRadius) + dz * dz);
		if (std::isfinite(rr)) {
			sumr += rr;
			countr++;
		}
	}
	if (countr > 0)
		out.minorRadius = sumr / countr;

	if (!std::isfinite(out.majorRadius) || out.majorRadius <= 1e-6)
		out.majorRadius = 1.0;
	if (!std::isfinite(out.minorRadius) || out.minorRadius <= 1e-6)
		out.minorRadius = 0.25;
	return out;
}

SaddleParams estimateSaddleParams(const std::vector<Vec3> &verts) {
	SaddleParams out;
	if (verts.empty())
		return out;

	const ObjNormalizeTransform t = computeNormalizeTransform(verts);
	out.center = t.center;

	double num = 0.0;
	double den = 0.0;
	for (const auto &v : verts) {
		const double x = v.x - out.center.x;
		const double y = v.y - out.center.y;
		const double z = v.z - out.center.z;
		const double txy = x * x - y * y;
		if (std::isfinite(txy) && std::isfinite(z)) {
			num += txy * z;
			den += txy * txy;
		}
	}
	if (den > 1e-12)
		out.a = num / den;
	if (!std::isfinite(out.a))
		out.a = 0.5;
	return out;
}

Vec3 applyNormalize(const ObjNormalizeTransform &t, const Vec3 &p) {
	return vmul(vsub(p, t.center), t.scale);
}