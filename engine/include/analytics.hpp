#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include "common.hpp"

// analytics.hpp
// Single-file "analytics" module (header-style) included by main.cpp.
// produces analytic (continuous) geodesic polylines for a small set of simple parametric surfaces.

// NOTE: Vec3/jsonEscape come from common.hpp (single shared definition).

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

static inline Vec3 vadd(const Vec3 &a, const Vec3 &b) {
	return {a.x + b.x, a.y + b.y, a.z + b.z};
}

static inline Vec3 vsub(const Vec3 &a, const Vec3 &b) {
	return {a.x - b.x, a.y - b.y, a.z - b.z};
}

static inline Vec3 vmul(const Vec3 &a, double s) {
	return {a.x * s, a.y * s, a.z * s};
}

static inline double vdot(const Vec3 &a, const Vec3 &b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline double vlen(const Vec3 &a) { return std::sqrt(vdot(a, a)); }

static inline Vec3 vnormalize(const Vec3 &a) {
	double l = vlen(a);
	if (l <= 1e-12)
		return {0, 0, 0};
	return {a.x / l, a.y / l, a.z / l};
}

static inline Vec3 vcross(const Vec3 &a, const Vec3 &b) {
	return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
	        a.x * b.y - a.y * b.x};
}

static inline Vec3 vlerp(const Vec3 &a, const Vec3 &b, double t) {
	return vadd(vmul(a, 1.0 - t), vmul(b, t));
}

static inline double clampd(double v, double lo, double hi) {
	return std::max(lo, std::min(hi, v));
}

static inline double vdist(const Vec3 &a, const Vec3 &b) {
	return vlen(vsub(a, b));
}

struct Metric2 {
	double g00 = 1.0;
	double g01 = 0.0;
	double g11 = 1.0;
	double inv00 = 1.0;
	double inv01 = 0.0;
	double inv11 = 1.0;
};

static inline Metric2 computeMetric(const ParamSurface &surf, double u,
                                    double v) {
	const double h = 1e-4;
	const Vec3 r = surf.eval(u, v);
	const Vec3 ru = vsub(surf.eval(u + h, v), r);
	const Vec3 rv = vsub(surf.eval(u, v + h), r);
	const Vec3 ru2 = vmul(ru, 1.0 / h);
	const Vec3 rv2 = vmul(rv, 1.0 / h);
	Metric2 m;
	m.g00 = vdot(ru2, ru2);
	m.g01 = vdot(ru2, rv2);
	m.g11 = vdot(rv2, rv2);
	const double det = m.g00 * m.g11 - m.g01 * m.g01;
	if (std::fabs(det) > 1e-12) {
		m.inv00 = m.g11 / det;
		m.inv01 = -m.g01 / det;
		m.inv11 = m.g00 / det;
	}
	return m;
}

struct Christoffel2 {
	// Gamma^u_{uu}, Gamma^u_{uv}, Gamma^u_{vv}, Gamma^v_{uu}, Gamma^v_{uv},
	// Gamma^v_{vv}
	double gu_uu = 0.0;
	double gu_uv = 0.0;
	double gu_vv = 0.0;
	double gv_uu = 0.0;
	double gv_uv = 0.0;
	double gv_vv = 0.0;
};

static inline Christoffel2 computeChristoffel(const ParamSurface &surf,
                                              double u, double v) {
	const double h = 1e-4;
	const Metric2 m = computeMetric(surf, u, v);
	const Metric2 mu = computeMetric(surf, u + h, v);
	const Metric2 mv = computeMetric(surf, u, v + h);

	const double dE_du = (mu.g00 - m.g00) / h;
	const double dF_du = (mu.g01 - m.g01) / h;
	const double dG_du = (mu.g11 - m.g11) / h;
	const double dE_dv = (mv.g00 - m.g00) / h;
	const double dF_dv = (mv.g01 - m.g01) / h;
	const double dG_dv = (mv.g11 - m.g11) / h;

	// g_ij derivatives: g00=E, g01=F, g11=G
	const double g00_u = dE_du;
	const double g01_u = dF_du;
	const double g11_u = dG_du;
	const double g00_v = dE_dv;
	const double g01_v = dF_dv;
	const double g11_v = dG_dv;

	Christoffel2 c;
	// Using Gamma^k_{ij} = 0.5 * g^{kl} (∂_i g_{jl} + ∂_j g_{il} - ∂_l g_{ij})
	// k = u (0), v (1)
	// i,j in {u,v}
	// For k=u:
	const double inv00 = m.inv00;
	const double inv01 = m.inv01;
	const double inv11 = m.inv11;

	const double guuu = 0.5 * (inv00 * (g00_u) + inv01 * (2.0 * g01_u - g00_v));
	const double guuv = 0.5 * (inv00 * (g00_v) + inv01 * (g11_u));
	const double guvv = 0.5 * (inv00 * (2.0 * g01_v - g11_u) + inv01 * (g11_v));

	const double gvuu = 0.5 * (inv01 * (g00_u) + inv11 * (2.0 * g01_u - g00_v));
	const double gvuv = 0.5 * (inv01 * (g00_v) + inv11 * (g11_u));
	const double gvvg = 0.5 * (inv01 * (2.0 * g01_v - g11_u) + inv11 * (g11_v));

	c.gu_uu = guuu;
	c.gu_uv = guuv;
	c.gu_vv = guvv;
	c.gv_uu = gvuu;
	c.gv_uv = gvuv;
	c.gv_vv = gvvg;
	return c;
}

struct GeodesicState {
	double u;
	double v;
	double du;
	double dv;
};

static inline GeodesicState geodesicRhs(const ParamSurface &surf,
                                        const GeodesicState &s) {
	const Christoffel2 c = computeChristoffel(surf, s.u, s.v);
	GeodesicState ds;
	ds.u = s.du;
	ds.v = s.dv;
	ds.du = -(c.gu_uu * s.du * s.du + 2.0 * c.gu_uv * s.du * s.dv +
	          c.gu_vv * s.dv * s.dv);
	ds.dv = -(c.gv_uu * s.du * s.du + 2.0 * c.gv_uv * s.du * s.dv +
	          c.gv_vv * s.dv * s.dv);
	return ds;
}

static inline GeodesicState rk4Step(const ParamSurface &surf,
                                    const GeodesicState &s, double h) {
	const GeodesicState k1 = geodesicRhs(surf, s);
	const GeodesicState s2{s.u + 0.5 * h * k1.u, s.v + 0.5 * h * k1.v,
	                       s.du + 0.5 * h * k1.du, s.dv + 0.5 * h * k1.dv};
	const GeodesicState k2 = geodesicRhs(surf, s2);
	const GeodesicState s3{s.u + 0.5 * h * k2.u, s.v + 0.5 * h * k2.v,
	                       s.du + 0.5 * h * k2.du, s.dv + 0.5 * h * k2.dv};
	const GeodesicState k3 = geodesicRhs(surf, s3);
	const GeodesicState s4{s.u + h * k3.u, s.v + h * k3.v, s.du + h * k3.du,
	                       s.dv + h * k3.dv};
	const GeodesicState k4 = geodesicRhs(surf, s4);

	GeodesicState out = s;
	out.u += (h / 6.0) * (k1.u + 2.0 * k2.u + 2.0 * k3.u + k4.u);
	out.v += (h / 6.0) * (k1.v + 2.0 * k2.v + 2.0 * k3.v + k4.v);
	out.du += (h / 6.0) * (k1.du + 2.0 * k2.du + 2.0 * k3.du + k4.du);
	out.dv += (h / 6.0) * (k1.dv + 2.0 * k2.dv + 2.0 * k3.dv + k4.dv);
	return out;
}

static inline std::vector<GeodesicState>
integrateGeodesic(const ParamSurface &surf, const GeodesicState &start,
                  int steps) {
	std::vector<GeodesicState> out;
	out.reserve(steps + 1);
	GeodesicState s = start;
	const double h = 1.0 / std::max(1, steps);
	out.push_back(s);
	for (int i = 0; i < steps; i++) {
		s = rk4Step(surf, s, h);
		out.push_back(s);
	}
	return out;
}

static inline bool solveShooting(const ParamSurface &surf, double u0, double v0,
                                 double u1, double v1, double &du0,
                                 double &dv0) {
	const int steps = 160;
	for (int iter = 0; iter < 8; iter++) {
		const GeodesicState s0{u0, v0, du0, dv0};
		auto path = integrateGeodesic(surf, s0, steps);
		const GeodesicState end = path.back();
		const double errU = end.u - u1;
		const double errV = end.v - v1;
		const double errMag = std::sqrt(errU * errU + errV * errV);
		if (errMag < 1e-3)
			return true;

		const double eps = 1e-3;
		const GeodesicState s0u{u0, v0, du0 + eps, dv0};
		const GeodesicState s0v{u0, v0, du0, dv0 + eps};
		const auto endU = integrateGeodesic(surf, s0u, steps).back();
		const auto endV = integrateGeodesic(surf, s0v, steps).back();

		const double a00 = (endU.u - end.u) / eps;
		const double a01 = (endV.u - end.u) / eps;
		const double a10 = (endU.v - end.v) / eps;
		const double a11 = (endV.v - end.v) / eps;

		const double det = a00 * a11 - a01 * a10;
		if (std::fabs(det) < 1e-10)
			break;

		const double du = (-errU * a11 + errV * a01) / det;
		const double dv = (errU * a10 - errV * a00) / det;
		du0 += du;
		dv0 += dv;
	}
	return false;
}

static inline ObjNormalizeTransform
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

static inline TorusParams estimateTorusParams(const std::vector<Vec3> &verts) {
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

static inline SaddleParams
estimateSaddleParams(const std::vector<Vec3> &verts) {
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

static inline Vec3 applyNormalize(const ObjNormalizeTransform &t,
                                  const Vec3 &p) {
	return vmul(vsub(p, t.center), t.scale);
}

static inline double cotangent(const Vec3 &a, const Vec3 &b, const Vec3 &c) {
	// cotangent of angle at a in triangle (a,b,c)
	Vec3 u = vsub(b, a);
	Vec3 v = vsub(c, a);
	Vec3 cr = vcross(u, v);
	const double denom = vlen(cr);
	if (denom <= 1e-12)
		return 0.0;
	return vdot(u, v) / denom;
}

static inline bool
conjugateGradient(const std::function<void(const std::vector<double> &,
                                           std::vector<double> &)> &applyA,
                  const std::vector<double> &b, std::vector<double> &x,
                  int maxIter, double tol) {
	const size_t n = b.size();
	std::vector<double> r(n, 0.0), p(n, 0.0), Ap(n, 0.0);
	applyA(x, Ap);
	for (size_t i = 0; i < n; i++) {
		r[i] = b[i] - Ap[i];
		p[i] = r[i];
	}
	double rsold = 0.0;
	for (double v : r)
		rsold += v * v;
	if (std::sqrt(rsold) < tol)
		return true;
	for (int iter = 0; iter < maxIter; iter++) {
		applyA(p, Ap);
		double alphaDen = 0.0;
		for (size_t i = 0; i < n; i++)
			alphaDen += p[i] * Ap[i];
		if (std::fabs(alphaDen) < 1e-20)
			break;
		const double alpha = rsold / alphaDen;
		for (size_t i = 0; i < n; i++)
			x[i] += alpha * p[i];
		for (size_t i = 0; i < n; i++)
			r[i] -= alpha * Ap[i];
		double rsnew = 0.0;
		for (double v : r)
			rsnew += v * v;
		if (std::sqrt(rsnew) < tol)
			return true;
		const double beta = rsnew / rsold;
		for (size_t i = 0; i < n; i++)
			p[i] = r[i] + beta * p[i];
		rsold = rsnew;
	}
	return false;
}

static inline AnalyticsCurve
makeHeatMethodGeodesic(const std::vector<Vec3> &verts,
                       const std::vector<Face> &faces, int startId, int endId) {
	AnalyticsCurve c;
	c.name = "heat_geodesic";
	const int n = static_cast<int>(verts.size());
	if (n <= 0 || startId < 0 || endId < 0 || startId >= n || endId >= n) {
		return c;
	}

	std::vector<double> mass(n, 0.0);
	std::vector<std::unordered_map<int, double>> weights(n);
	std::vector<std::vector<int>> neighbors(n);

	double edgeSum = 0.0;
	int edgeCount = 0;

	for (const auto &f : faces) {
		const int i = f[0];
		const int j = f[1];
		const int k = f[2];
		if (i < 0 || j < 0 || k < 0 || i >= n || j >= n || k >= n)
			continue;

		const Vec3 &pi = verts[i];
		const Vec3 &pj = verts[j];
		const Vec3 &pk = verts[k];
		Vec3 nrm = vcross(vsub(pj, pi), vsub(pk, pi));
		const double area = 0.5 * vlen(nrm);
		if (!std::isfinite(area) || area <= 1e-12)
			continue;
		mass[i] += area / 3.0;
		mass[j] += area / 3.0;
		mass[k] += area / 3.0;

		const double cot_i = cotangent(pi, pj, pk);
		const double cot_j = cotangent(pj, pk, pi);
		const double cot_k = cotangent(pk, pi, pj);

		const double w_ij = 0.5 * cot_k;
		const double w_jk = 0.5 * cot_i;
		const double w_ki = 0.5 * cot_j;

		weights[i][j] += w_ij;
		weights[j][i] += w_ij;
		weights[j][k] += w_jk;
		weights[k][j] += w_jk;
		weights[k][i] += w_ki;
		weights[i][k] += w_ki;

		edgeSum += vdist(pi, pj) + vdist(pj, pk) + vdist(pk, pi);
		edgeCount += 3;
	}

	for (int i = 0; i < n; i++) {
		neighbors[i].reserve(weights[i].size());
		for (const auto &kv : weights[i]) {
			neighbors[i].push_back(kv.first);
		}
	}

	const double h = (edgeCount > 0) ? (edgeSum / edgeCount) : 1.0;
	const double t = h * h;

	std::vector<double> b(n, 0.0), u(n, 0.0);
	if (mass[startId] <= 1e-12)
		return c;
	b[startId] = mass[startId];

	auto applyL = [&](const std::vector<double> &x, std::vector<double> &out) {
		out.assign(n, 0.0);
		for (int i = 0; i < n; i++) {
			double sum = 0.0;
			for (const auto &kv : weights[i]) {
				sum += kv.second * (x[i] - x[kv.first]);
			}
			out[i] = sum;
		}
	};

	auto applyHeat = [&](const std::vector<double> &x,
	                     std::vector<double> &out) {
		std::vector<double> Lx;
		applyL(x, Lx);
		out.assign(n, 0.0);
		for (int i = 0; i < n; i++) {
			out[i] = mass[i] * x[i] - t * Lx[i];
		}
	};

	const bool okHeat = conjugateGradient(applyHeat, b, u, 600, 1e-6);

	// Compute vector field X on faces
	std::vector<double> div(n, 0.0);
	for (const auto &f : faces) {
		const int i = f[0];
		const int j = f[1];
		const int k = f[2];
		if (i < 0 || j < 0 || k < 0 || i >= n || j >= n || k >= n)
			continue;

		const Vec3 &pi = verts[i];
		const Vec3 &pj = verts[j];
		const Vec3 &pk = verts[k];
		Vec3 nrm = vcross(vsub(pj, pi), vsub(pk, pi));
		const double area2 = vlen(nrm);
		if (area2 <= 1e-12)
			continue;

		const Vec3 grad_phi_i = vmul(vcross(nrm, vsub(pk, pj)), 1.0 / area2);
		const Vec3 grad_phi_j = vmul(vcross(nrm, vsub(pi, pk)), 1.0 / area2);
		const Vec3 grad_phi_k = vmul(vcross(nrm, vsub(pj, pi)), 1.0 / area2);

		Vec3 grad_u = vadd(vadd(vmul(grad_phi_i, u[i]), vmul(grad_phi_j, u[j])),
		                   vmul(grad_phi_k, u[k]));
		double gradLen = vlen(grad_u);
		if (gradLen <= 1e-12)
			continue;
		Vec3 X = vmul(grad_u, -1.0 / gradLen);

		const double cot_i = cotangent(pi, pj, pk);
		const double cot_j = cotangent(pj, pk, pi);
		const double cot_k = cotangent(pk, pi, pj);

		div[i] += 0.5 * (cot_j * vdot(vsub(pk, pi), X) +
		                 cot_k * vdot(vsub(pj, pi), X));
		div[j] += 0.5 * (cot_k * vdot(vsub(pi, pj), X) +
		                 cot_i * vdot(vsub(pk, pj), X));
		div[k] += 0.5 * (cot_i * vdot(vsub(pj, pk), X) +
		                 cot_j * vdot(vsub(pi, pk), X));
	}

	std::vector<double> phi(n, 0.0);
	auto applyLConstrained = [&](const std::vector<double> &x,
	                             std::vector<double> &out) {
		out.assign(n, 0.0);
		for (int i = 0; i < n; i++) {
			if (i == startId) {
				out[i] = x[i];
				continue;
			}
			double sum = 0.0;
			for (const auto &kv : weights[i]) {
				sum += kv.second * (x[i] - x[kv.first]);
			}
			out[i] = sum;
		}
	};

	std::vector<double> rhs = div;
	rhs[startId] = 0.0;
	const bool okPoisson =
	    conjugateGradient(applyLConstrained, rhs, phi, 1000, 1e-6);

	// Shift so source is zero and ensure non-negative
	double minPhi = std::numeric_limits<double>::infinity();
	for (double v : phi)
		minPhi = std::min(minPhi, v);
	for (double &v : phi)
		v -= minPhi;

	// Extract path by greedy descent on vertices
	std::vector<int> path;
	path.push_back(endId);
	int current = endId;
	std::vector<char> visited(n, 0);
	visited[current] = 1;
	const double eps = 1e-9;
	for (int step = 0; step < n * 3; step++) {
		if (current == startId)
			break;
		int best = -1;
		double bestVal = phi[current];
		for (int nb : neighbors[current]) {
			if (phi[nb] + eps < bestVal) {
				bestVal = phi[nb];
				best = nb;
			}
		}
		if (best == -1) {
			// allow non-decreasing move to escape plateaus
			for (int nb : neighbors[current]) {
				if (!visited[nb] && phi[nb] < bestVal + 1e-6) {
					bestVal = phi[nb];
					best = nb;
				}
			}
		}
		if (best == -1)
			break;
		path.push_back(best);
		current = best;
		visited[current] = 1;
	}

	if (path.back() != startId) {
		// Fallback: Dijkstra on edge lengths
		std::vector<double> distv(n, std::numeric_limits<double>::infinity());
		std::vector<int> parent(n, -1);
		distv[startId] = 0.0;
		using Node = std::pair<double, int>;
		std::priority_queue<Node, std::vector<Node>, std::greater<>> pq;
		pq.push({0.0, startId});
		while (!pq.empty()) {
			const auto [d, uidx] = pq.top();
			pq.pop();
			if (d > distv[uidx])
				continue;
			if (uidx == endId)
				break;
			for (int nb : neighbors[uidx]) {
				const double w = vdist(verts[uidx], verts[nb]);
				if (distv[uidx] + w < distv[nb]) {
					distv[nb] = distv[uidx] + w;
					parent[nb] = uidx;
					pq.push({distv[nb], nb});
				}
			}
		}
		if (parent[endId] == -1 && startId != endId)
			return c;
		path.clear();
		for (int v = endId; v != -1; v = parent[v]) {
			path.push_back(v);
		}
		std::reverse(path.begin(), path.end());
	}

	std::reverse(path.begin(), path.end());
	for (int idx : path) {
		c.points.push_back(verts[idx]);
	}

	for (size_t i = 1; i < c.points.size(); i++) {
		c.length += vlen(vsub(c.points[i], c.points[i - 1]));
	}
	return c;
}

static inline std::string basenameOnly(const std::string &path) {
	std::string s = path;
	for (auto &ch : s) {
		if (ch == '\\')
			ch = '/';
	}
	const auto slash = s.find_last_of('/');
	return (slash == std::string::npos) ? s : s.substr(slash + 1);
}

static inline std::string lowerAscii(std::string s) {
	for (auto &c : s) {
		if (c >= 'A' && c <= 'Z')
			c = char(c - 'A' + 'a');
	}
	return s;
}

static inline AnalyticsCurve makePlaneGeodesic(const Vec3 &p1, const Vec3 &p2,
                                               int samples) {
	AnalyticsCurve c;
	c.name = "plane_straight_line";
	c.points.reserve(std::max(2, samples));
	const int n = std::max(2, samples);
	for (int i = 0; i < n; i++) {
		const double t = (n == 1) ? 0.0 : (double)i / (double)(n - 1);
		c.points.push_back(vadd(vmul(p1, 1.0 - t), vmul(p2, t)));
	}
	c.length = vlen(vsub(p2, p1));
	return c;
}

static inline AnalyticsCurve
makeSphereGreatCircle(const Vec3 &p1, const Vec3 &p2, int samples) {
	AnalyticsCurve c;
	c.name = "sphere_great_circle";
	c.points.reserve(std::max(2, samples));

	const double r1 = vlen(p1);
	const double r2 = vlen(p2);
	const double r =
	    (r1 > 1e-12 && r2 > 1e-12) ? (0.5 * (r1 + r2)) : std::max(r1, r2);

	Vec3 a = (r1 > 1e-12) ? vmul(p1, 1.0 / r1) : Vec3{0, 0, 1};
	Vec3 b = (r2 > 1e-12) ? vmul(p2, 1.0 / r2) : Vec3{0, 0, 1};

	double dot = vdot(a, b);
	dot = std::max(-1.0, std::min(1.0, dot));
	const double theta = std::acos(dot);
	const double sinTheta = std::sin(theta);

	const int n = std::max(2, samples);

	// Special-case antipodal points.
	// Renormalized lerp between a and -a hits the zero vector at t=0.5, which
	// collapses to the origin and draws a line through the center.
	const bool nearAntipodal = (M_PI - theta) <= 1e-5;
	const bool nearIdentical = theta <= 1e-8;
	const bool useLerp =
	    (!nearAntipodal) && ((sinTheta <= 1e-6) || !std::isfinite(sinTheta));

	if (nearIdentical) {
		for (int i = 0; i < n; i++) {
			c.points.push_back(vmul(a, r));
		}
		c.length = 0.0;
		return c;
	}

	if (nearAntipodal) {
		// Choose an arbitrary axis perpendicular to a to define one of the
		// infinitely many great circles connecting antipodal points.
		Vec3 ref = (std::fabs(a.x) < 0.9) ? Vec3{1, 0, 0} : Vec3{0, 1, 0};
		Vec3 u = vnormalize(vcross(a, ref));
		if (vlen(u) <= 1e-8) {
			ref = Vec3{0, 0, 1};
			u = vnormalize(vcross(a, ref));
		}
		for (int i = 0; i < n; i++) {
			const double t = (double)i / (double)(n - 1);
			const double ang = M_PI * t;
			const double ca = std::cos(ang);
			const double sa = std::sin(ang);
			Vec3 p = vadd(vmul(a, ca), vmul(u, sa));
			c.points.push_back(vmul(p, r));
		}
		c.length = r * M_PI;
		return c;
	}

	for (int i = 0; i < n; i++) {
		const double t = (double)i / (double)(n - 1);
		Vec3 u;
		if (useLerp) {
			u = vnormalize(vadd(vmul(a, 1.0 - t), vmul(b, t)));
		} else {
			const double w1 = std::sin((1.0 - t) * theta) / sinTheta;
			const double w2 = std::sin(t * theta) / sinTheta;
			u = vadd(vmul(a, w1), vmul(b, w2));
		}
		c.points.push_back(vmul(u, r));
	}

	c.length = r * theta;
	return c;
}

static inline AnalyticsCurve makeTorusApproxGeodesic(const Vec3 &p1,
                                                     const Vec3 &p2,
                                                     const TorusParams &torus,
                                                     int samples) {
	AnalyticsCurve c;
	c.name = "torus_geodesic";
	const int n = std::max(2, samples);
	c.points.reserve(n);

	auto toUV = [&](const Vec3 &p) {
		const double x = p.x - torus.center.x;
		const double y = p.y - torus.center.y;
		const double z = p.z - torus.center.z;
		const double u = std::atan2(y, x);
		const double rho = std::sqrt(x * x + y * y);
		const double v = std::atan2(z, rho - torus.majorRadius);
		return std::pair<double, double>(u, v);
	};

	auto wrapDelta = [&](double a, double b) {
		const double twoPi = 2.0 * M_PI;
		double delta = std::remainder(b - a, twoPi);
		return a + delta;
	};

	const auto uv1 = toUV(p1);
	const auto uv2 = toUV(p2);
	const double u1 = uv1.first;
	const double v1 = uv1.second;
	const double u2 = wrapDelta(u1, uv2.first);
	const double v2 = wrapDelta(v1, uv2.second);

	ParamSurface surf;
	surf.eval = [&](double u, double v) {
		const double cu = std::cos(u);
		const double su = std::sin(u);
		const double cv = std::cos(v);
		const double sv = std::sin(v);
		const double x = (torus.majorRadius + torus.minorRadius * cv) * cu;
		const double y = (torus.majorRadius + torus.minorRadius * cv) * su;
		const double z = torus.minorRadius * sv;
		return Vec3{x + torus.center.x, y + torus.center.y, z + torus.center.z};
	};

	double du0 = u2 - u1;
	double dv0 = v2 - v1;
	bool ok = solveShooting(surf, u1, v1, u2, v2, du0, dv0);
	const GeodesicState s0{u1, v1, du0, dv0};
	const auto path = integrateGeodesic(surf, s0, n - 1);
	for (const auto &s : path) {
		c.points.push_back(surf.eval(s.u, s.v));
	}

	if (!ok && c.points.size() >= 2) {
		// fallback: straight param interpolation if shooting fails
		c.points.clear();
		for (int i = 0; i < n; i++) {
			const double t = (n == 1) ? 0.0 : (double)i / (double)(n - 1);
			const double u = u1 + (u2 - u1) * t;
			const double v = v1 + (v2 - v1) * t;
			c.points.push_back(surf.eval(u, v));
		}
	}
	if (!c.points.empty()) {
		c.points.front() = p1;
		c.points.back() = p2;
	}

	double length = 0.0;
	for (size_t i = 1; i < c.points.size(); i++) {
		length += vlen(vsub(c.points[i], c.points[i - 1]));
	}
	c.length = length;
	return c;
}

static inline AnalyticsCurve
makeSaddleApproxGeodesic(const Vec3 &p1, const Vec3 &p2,
                         const SaddleParams &saddle, int samples) {
	AnalyticsCurve c;
	c.name = "saddle_geodesic";
	const int n = std::max(2, samples);
	c.points.reserve(n);

	ParamSurface surf;
	surf.eval = [&](double u, double v) {
		const double dx = u;
		const double dy = v;
		const double z = saddle.center.z + saddle.a * (dx * dx - dy * dy);
		return Vec3{u + saddle.center.x, v + saddle.center.y, z};
	};

	const double u1 = p1.x - saddle.center.x;
	const double v1 = p1.y - saddle.center.y;
	const double u2 = p2.x - saddle.center.x;
	const double v2 = p2.y - saddle.center.y;

	double du0 = u2 - u1;
	double dv0 = v2 - v1;
	bool ok = solveShooting(surf, u1, v1, u2, v2, du0, dv0);
	const GeodesicState s0{u1, v1, du0, dv0};
	const auto path = integrateGeodesic(surf, s0, n - 1);
	for (const auto &s : path) {
		c.points.push_back(surf.eval(s.u, s.v));
	}

	if (!ok && c.points.size() >= 2) {
		c.points.clear();
		for (int i = 0; i < n; i++) {
			const double t = (n == 1) ? 0.0 : (double)i / (double)(n - 1);
			const double u = u1 + (u2 - u1) * t;
			const double v = v1 + (v2 - v1) * t;
			c.points.push_back(surf.eval(u, v));
		}
	}
	if (!c.points.empty()) {
		c.points.front() = p1;
		c.points.back() = p2;
	}

	double length = 0.0;
	for (size_t i = 1; i < c.points.size(); i++) {
		length += vlen(vsub(c.points[i], c.points[i - 1]));
	}
	c.length = length;
	return c;
}

static inline AnalyticsResult
computeAnalyticsForModel(const std::string &inputFileName, int startId,
                         int endId, const std::vector<Vec3> &objVertices,
                         const std::vector<Face> &faces) {
	AnalyticsResult out;
	out.inputFileName = inputFileName;
	out.startId = startId;
	out.endId = endId;

	if (objVertices.empty()) {
		out.error = "No vertices loaded from OBJ";
		return out;
	}
	if (startId < 0 || endId < 0 || startId >= (int)objVertices.size() ||
	    endId >= (int)objVertices.size()) {
		out.error = "startId/endId out of range";
		return out;
	}

	const ObjNormalizeTransform t = computeNormalizeTransform(objVertices);
	const Vec3 p1 = applyNormalize(t, objVertices[(size_t)startId]);
	const Vec3 p2 = applyNormalize(t, objVertices[(size_t)endId]);
	const double lengthScale = (t.scale > 1e-12) ? (1.0 / t.scale) : 1.0;

	std::vector<Vec3> normalizedVerts;
	normalizedVerts.reserve(objVertices.size());
	for (const auto &v : objVertices) {
		normalizedVerts.push_back(applyNormalize(t, v));
	}

	const std::string name = lowerAscii(basenameOnly(inputFileName));

	// Option 1 (pragmatic): infer the analytic surface type from the OBJ name.
	// We only implement a couple of "simple surfaces" for now.
	if (name.find("plane") != std::string::npos) {
		out.surfaceType = "plane";
		out.curves.push_back(makePlaneGeodesic(p1, p2, 64));
		if (!out.curves.empty()) {
			out.curves.back().length *= lengthScale;
		}
		return out;
	}

	if (name.find("sphere") != std::string::npos) {
		out.surfaceType = "sphere";
		out.curves.push_back(makeSphereGreatCircle(p1, p2, 128));
		if (!out.curves.empty()) {
			out.curves.back().length *= lengthScale;
		}
		return out;
	}

	if (name.find("torus") != std::string::npos ||
	    name.find("donut") != std::string::npos) {
		out.surfaceType = "torus";
		const TorusParams torus = estimateTorusParams(normalizedVerts);
		out.curves.push_back(makeTorusApproxGeodesic(p1, p2, torus, 160));
		if (!out.curves.empty()) {
			out.curves.back().length *= lengthScale;
		}
		return out;
	}

	if (name.find("saddle") != std::string::npos) {
		out.surfaceType = "saddle";
		const SaddleParams saddle = estimateSaddleParams(normalizedVerts);
		out.curves.push_back(makeSaddleApproxGeodesic(p1, p2, saddle, 160));
		if (!out.curves.empty()) {
			out.curves.back().length *= lengthScale;
		}
		return out;
	}

	// Heat method for any mesh.
	if (!faces.empty()) {
		out.surfaceType = "mesh";
		AnalyticsCurve heat =
		    makeHeatMethodGeodesic(normalizedVerts, faces, startId, endId);
		if (!heat.points.empty()) {
			heat.length *= lengthScale;
			out.curves.push_back(std::move(heat));
			return out;
		}
		out.error = "Heat method failed to produce a path";
		return out;
	}

	out.surfaceType = "unsupported";
	out.error = "Analytics currently supports plane.obj, sphere.obj, "
	            "donut.obj, saddle.obj, or heat method on triangle meshes";
	return out;
}

static inline AnalyticsResult
computeHeatForModel(const std::string &inputFileName, int startId, int endId,
                    const std::vector<Vec3> &objVertices,
                    const std::vector<Face> &faces) {
	AnalyticsResult out;
	out.inputFileName = inputFileName;
	out.startId = startId;
	out.endId = endId;
	out.surfaceType = "mesh";

	if (objVertices.empty()) {
		out.error = "No vertices loaded from OBJ";
		return out;
	}
	if (faces.empty()) {
		out.error = "No faces loaded from OBJ";
		return out;
	}
	if (startId < 0 || endId < 0 || startId >= (int)objVertices.size() ||
	    endId >= (int)objVertices.size()) {
		out.error = "startId/endId out of range";
		return out;
	}

	const ObjNormalizeTransform t = computeNormalizeTransform(objVertices);
	std::vector<Vec3> normalizedVerts;
	normalizedVerts.reserve(objVertices.size());
	for (const auto &v : objVertices) {
		normalizedVerts.push_back(applyNormalize(t, v));
	}
	const double lengthScale = (t.scale > 1e-12) ? (1.0 / t.scale) : 1.0;

	AnalyticsCurve heat =
	    makeHeatMethodGeodesic(normalizedVerts, faces, startId, endId);
	if (heat.points.empty()) {
		out.error = "Heat method failed to produce a path";
		return out;
	}
	heat.length *= lengthScale;
	out.curves.push_back(std::move(heat));
	return out;
}

static inline void writeAnalyticsJSON(const std::string &outputFilename,
                                      const std::string &outputPath,
                                      const AnalyticsResult &res) {
	std::string fullPath = outputPath + outputFilename;
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
	file << "  \"surfaceType\": \"" << jsonEscape(res.surfaceType) << "\",\n";
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
