#include "../../include/geodesic_lab/analytics/analytic_surface_curves.hpp"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

#include "../../include/geodesic_lab/types/christoffel2.hpp"
#include "../../include/geodesic_lab/types/geodesic_state.hpp"
#include "../../include/geodesic_lab/types/metric2.hpp"
#include "../../include/geodesic_lab/types/vec3_ops.hpp"

namespace {

Metric2 computeMetric(const ParamSurface &surf, double u, double v) {
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

Christoffel2 computeChristoffel(const ParamSurface &surf, double u, double v) {
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

	const double g00_u = dE_du;
	const double g01_u = dF_du;
	const double g11_u = dG_du;
	const double g00_v = dE_dv;
	const double g01_v = dF_dv;
	const double g11_v = dG_dv;

	Christoffel2 c;
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

GeodesicState geodesicRhs(const ParamSurface &surf, const GeodesicState &s) {
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

GeodesicState rk4Step(const ParamSurface &surf, const GeodesicState &s,
	                  double h) {
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

std::vector<GeodesicState> integrateGeodesic(const ParamSurface &surf,
	                                         const GeodesicState &start,
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

bool solveShooting(const ParamSurface &surf, double u0, double v0, double u1,
	              double v1, double &du0, double &dv0) {
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

} // namespace

AnalyticsCurve makePlaneGeodesic(const Vec3 &p1, const Vec3 &p2,
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

AnalyticsCurve makeSphereGreatCircle(const Vec3 &p1, const Vec3 &p2,
	                                 int samples) {
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

AnalyticsCurve makeTorusApproxGeodesic(const Vec3 &p1, const Vec3 &p2,
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

AnalyticsCurve makeSaddleApproxGeodesic(const Vec3 &p1, const Vec3 &p2,
	                                   const SaddleParams &saddle,
	                                   int samples) {
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