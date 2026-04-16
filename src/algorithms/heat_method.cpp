#include "../../include/geodesic_lab/algorithms/heat_method.hpp"
#include "../../include/geodesic_lab/types/vec3_ops.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <queue>
#include <unordered_map>
#include <vector>

namespace {

double cotangent(const Vec3 &a, const Vec3 &b, const Vec3 &c) {
	// cotangent of angle at a in triangle (a,b,c)
	const Vec3 u = vsub(b, a);
	const Vec3 v = vsub(c, a);
	const Vec3 cr = vcross(u, v);
	const double denom = vlen(cr);
	if (denom <= 1e-12)
		return 0.0;
	return vdot(u, v) / denom;
}

bool conjugateGradient(const std::function<void(const std::vector<double> &,
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

} // namespace

AnalyticsCurve makeHeatMethodGeodesic(const std::vector<Vec3> &verts,
                                      const std::vector<Face> &faces,
                                      int startId, int endId) {
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
		const Vec3 nrm = vcross(vsub(pj, pi), vsub(pk, pi));
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
	(void)okHeat;

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
		const Vec3 nrm = vcross(vsub(pj, pi), vsub(pk, pi));
		const double area2 = vlen(nrm);
		if (area2 <= 1e-12)
			continue;

		const Vec3 grad_phi_i = vmul(vcross(nrm, vsub(pk, pj)), 1.0 / area2);
		const Vec3 grad_phi_j = vmul(vcross(nrm, vsub(pi, pk)), 1.0 / area2);
		const Vec3 grad_phi_k = vmul(vcross(nrm, vsub(pj, pi)), 1.0 / area2);

		const Vec3 grad_u =
		    vadd(vadd(vmul(grad_phi_i, u[i]), vmul(grad_phi_j, u[j])),
		         vmul(grad_phi_k, u[k]));
		const double gradLen = vlen(grad_u);
		if (gradLen <= 1e-12)
			continue;
		const Vec3 X = vmul(grad_u, -1.0 / gradLen);

		const double cot_i = cotangent(pi, pj, pk);
		const double cot_j = cotangent(pj, pk, pi);
		const double cot_k = cotangent(pk, pi, pj);

		div[i] += 0.5 *
		          (cot_j * vdot(vsub(pk, pi), X) + cot_k * vdot(vsub(pj, pi), X));
		div[j] += 0.5 *
		          (cot_k * vdot(vsub(pi, pj), X) + cot_i * vdot(vsub(pk, pj), X));
		div[k] += 0.5 *
		          (cot_i * vdot(vsub(pj, pk), X) + cot_j * vdot(vsub(pi, pk), X));
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
	const bool okPoisson = conjugateGradient(applyLConstrained, rhs, phi, 1000, 1e-6);
	(void)okPoisson;

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
