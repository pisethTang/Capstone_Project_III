#include "../../include/geodesic_lab/algorithms/heat_method.hpp"
#include "../../include/geodesic_lab/types/vec3_ops.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <queue>
#include <unordered_map>
#include <vector>

#include <Eigen/Sparse>
#include <Eigen/SparseCholesky>

namespace {

struct EdgeKey {
	int a, b;
	bool operator==(const EdgeKey &o) const { return a == o.a && b == o.b; }
};

struct EdgeKeyHash {
	std::size_t operator()(const EdgeKey &k) const noexcept {
		return std::hash<int>()(k.a) ^ (std::hash<int>()(k.b) << 1);
	}
};

double cotangent(const Vec3 &a, const Vec3 &b, const Vec3 &c) {
	const Vec3 u = vsub(b, a);
	const Vec3 v = vsub(c, a);
	const Vec3 cr = vcross(u, v);
	const double denom = vlen(cr);
	if (denom <= 1e-12)
		return 0.0;
	return vdot(u, v) / denom;
}

void barycentric(const Vec3 &p, const Vec3 &a, const Vec3 &b, const Vec3 &c,
                 double &u, double &v, double &w) {
	Vec3 v0 = vsub(b, a);
	Vec3 v1 = vsub(c, a);
	Vec3 v2 = vsub(p, a);
	double d00 = vdot(v0, v0);
	double d01 = vdot(v0, v1);
	double d11 = vdot(v1, v1);
	double d20 = vdot(v2, v0);
	double d21 = vdot(v2, v1);
	double denom = d00 * d11 - d01 * d01;
	if (std::abs(denom) < 1e-12) {
		u = v = w = -1;
		return;
	}
	v = (d11 * d20 - d01 * d21) / denom;
	w = (d00 * d21 - d01 * d20) / denom;
	u = 1.0 - v - w;
}

// Given a triangle (a,b,c), a point p, and a direction d (already in triangle plane),
// find the exit edge and parametric distance t where ray p + t*d exits the triangle.
// exitEdge: 0=(b,c), 1=(a,c), 2=(a,b)
bool findTriangleExit(const Vec3 &p, const Vec3 &a, const Vec3 &b, const Vec3 &c,
                      const Vec3 &d, int &exitEdge, double &t) {
	if (vlen(d) < 1e-12)
		return false;

	double wa, wb, wc;
	barycentric(p, a, b, c, wa, wb, wc);

	double wa1, wb1, wc1;
	barycentric(vadd(p, d), a, b, c, wa1, wb1, wc1);
	double dw[3] = {wa1 - wa, wb1 - wb, wc1 - wc};
	double w[3]  = {wa, wb, wc};

	t = std::numeric_limits<double>::infinity();
	exitEdge = -1;
	for (int i = 0; i < 3; ++i) {
		if (dw[i] >= -1e-12)
			continue;
		double ti = -w[i] / dw[i];
		if (ti <= 1e-12)
			continue;
		bool valid = true;
		for (int j = 0; j < 3; ++j) {
			if (j == i)
				continue;
			if (w[j] + ti * dw[j] < -1e-9) {
				valid = false;
				break;
			}
		}
		if (valid && ti < t) {
			t = ti;
			exitEdge = i;
		}
	}
	return exitEdge >= 0 && std::isfinite(t);
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

	// ------------------------------------------------------------------
	// 1. Build cotan Laplacian weights, mass matrix, and adjacency
	// ------------------------------------------------------------------
	std::vector<double> mass(n, 0.0);
	std::vector<std::unordered_map<int, double>> weights(n);
	std::vector<std::vector<int>> neighbors(n);

	double edgeSum = 0.0;
	int edgeCount = 0;

	for (const auto &f : faces) {
		const int i = f[0], j = f[1], k = f[2];
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

	// ------------------------------------------------------------------
	// 2. Build Eigen sparse Laplacian L and lumped mass matrix M
	// ------------------------------------------------------------------
	std::vector<Eigen::Triplet<double>> triplets;
	triplets.reserve(n * 8);
	for (int i = 0; i < n; i++) {
		double diag = 0.0;
		for (const auto &kv : weights[i]) {
			int j = kv.first;
			double w = kv.second;
			triplets.emplace_back(i, j, -w);
			diag += w;
		}
		triplets.emplace_back(i, i, diag);
	}

	Eigen::SparseMatrix<double> L(n, n);
	L.setFromTriplets(triplets.begin(), triplets.end());
	L.makeCompressed();

	Eigen::SparseMatrix<double> M(n, n);
	{
		std::vector<Eigen::Triplet<double>> mt;
		mt.reserve(n);
		for (int i = 0; i < n; i++) {
			mt.emplace_back(i, i, mass[i]);
		}
		M.setFromTriplets(mt.begin(), mt.end());
		M.makeCompressed();
	}

	// ------------------------------------------------------------------
	// 3. Solve heat equation: (M + t*L) u = delta_source
	// ------------------------------------------------------------------
	Eigen::VectorXd b = Eigen::VectorXd::Zero(n);
	if (mass[startId] <= 1e-12)
		return c;
	b[startId] = mass[startId];

	Eigen::SparseMatrix<double> heatOp = M + t * L;
	for (int i = 0; i < n; i++) {
		heatOp.coeffRef(i, i) += 1e-10;
	}
	heatOp.makeCompressed();

	Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> heatSolver;
	heatSolver.compute(heatOp);
	if (heatSolver.info() != Eigen::Success)
		return c;
	Eigen::VectorXd u = heatSolver.solve(b);

	// ------------------------------------------------------------------
	// 4. Compute normalized vector field X per face and vertex divergence
	// ------------------------------------------------------------------
	std::vector<double> div(n, 0.0);
	for (const auto &f : faces) {
		const int i = f[0], j = f[1], k = f[2];
		if (i < 0 || j < 0 || k < 0 || i >= n || j >= n || k >= n)
			continue;

		const Vec3 &pi = verts[i];
		const Vec3 &pj = verts[j];
		const Vec3 &pk = verts[k];
		const Vec3 nrm = vcross(vsub(pj, pi), vsub(pk, pi));
		const double area2 = vlen(nrm);
		if (area2 <= 1e-12)
			continue;

		const double scale = 1.0 / (area2 * area2);
		const Vec3 grad_phi_i = vmul(vcross(nrm, vsub(pk, pj)), scale);
		const Vec3 grad_phi_j = vmul(vcross(nrm, vsub(pi, pk)), scale);
		const Vec3 grad_phi_k = vmul(vcross(nrm, vsub(pj, pi)), scale);

		const Vec3 grad_u = vadd(vadd(vmul(grad_phi_i, u[i]),
		                              vmul(grad_phi_j, u[j])),
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

	// ------------------------------------------------------------------
	// 5. Solve Poisson equation: L phi = divergence, regularized
	// ------------------------------------------------------------------
	Eigen::SparseMatrix<double> Lreg = L;
	for (int i = 0; i < n; i++) {
		Lreg.coeffRef(i, i) += 1e-6;
	}
	Lreg.makeCompressed();

	Eigen::VectorXd rhs(n);
	for (int i = 0; i < n; i++) {
		rhs[i] = div[i];
	}

	Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> poissonSolver;
	poissonSolver.compute(Lreg);
	if (poissonSolver.info() != Eigen::Success)
		return c;
	Eigen::VectorXd phi = poissonSolver.solve(rhs);

	// Shift so minimum distance is zero
	double minPhi = std::numeric_limits<double>::infinity();
	for (int i = 0; i < n; i++) {
		minPhi = std::min(minPhi, phi[i]);
	}
	for (int i = 0; i < n; i++) {
		phi[i] -= minPhi;
	}

	// ------------------------------------------------------------------
	// 6. Extract smooth geodesic path via gradient descent through triangles
	// ------------------------------------------------------------------
	std::vector<Vec3> pathPts;
	pathPts.reserve(4096);
	pathPts.push_back(verts[endId]);

	// Build edge -> face adjacency for triangle hopping
	std::unordered_map<EdgeKey, std::vector<int>, EdgeKeyHash> edgeFaces;
	for (size_t fi = 0; fi < faces.size(); ++fi) {
		const auto &f = faces[fi];
		for (int e = 0; e < 3; ++e) {
			int a = f[e];
			int b = f[(e + 1) % 3];
			EdgeKey ek{std::min(a, b), std::max(a, b)};
			edgeFaces[ek].push_back(static_cast<int>(fi));
		}
	}

	// Find initial triangle: among faces incident to endId, pick one where
	// -grad(phi) points into the triangle interior (has positive barycentric
	// rate for the vertex opposite endId).
	int currentTri = -1;
	{
		double bestScore = -std::numeric_limits<double>::infinity();
		for (size_t fi = 0; fi < faces.size(); ++fi) {
			const auto &f = faces[fi];
			int vi = -1;
			for (int k = 0; k < 3; ++k) {
				if (f[k] == endId) {
					vi = k;
					break;
				}
			}
			if (vi < 0)
				continue;

			const Vec3 &a = verts[f[0]];
			const Vec3 &b = verts[f[1]];
			const Vec3 &c = verts[f[2]];
			const Vec3 nrm = vcross(vsub(b, a), vsub(c, a));
			const double area2 = vlen(nrm);
			if (area2 <= 1e-12)
				continue;

			const double scale = 1.0 / (area2 * area2);
			const Vec3 g_i = vmul(vcross(nrm, vsub(c, b)), scale);
			const Vec3 g_j = vmul(vcross(nrm, vsub(a, c)), scale);
			const Vec3 g_k = vmul(vcross(nrm, vsub(b, a)), scale);
			const Vec3 grad = vadd(vadd(vmul(g_i, phi[f[0]]),
			                            vmul(g_j, phi[f[1]])),
			                       vmul(g_k, phi[f[2]]));

			Vec3 toCenter = vadd(vadd(a, b), c);
			toCenter = vmul(toCenter, 1.0 / 3.0);
			toCenter = vsub(toCenter, verts[endId]);
			double score = -vdot(grad, toCenter);
			if (score > bestScore) {
				bestScore = score;
				currentTri = static_cast<int>(fi);
			}
		}
	}

	const double stepSize = h * 0.25;
	const double stopDist = stepSize * 2.0;
	const int maxSteps = 20000;
	Vec3 pos = verts[endId];
	int steps = 0;

	while (currentTri >= 0 && steps < maxSteps) {
		if (vdist(pos, verts[startId]) < stopDist)
			break;

		const auto &f = faces[currentTri];
		const int i = f[0], j = f[1], k = f[2];
		const Vec3 &a = verts[i];
		const Vec3 &b = verts[j];
		const Vec3 &c = verts[k];

		// Compute gradient of phi in this triangle (constant per-triangle)
		const Vec3 nrm = vcross(vsub(b, a), vsub(c, a));
		const double area2 = vlen(nrm);
		if (area2 <= 1e-12) {
			currentTri = -1;
			break;
		}

		const double scale = 1.0 / (area2 * area2);
		const Vec3 g_i = vmul(vcross(nrm, vsub(c, b)), scale);
		const Vec3 g_j = vmul(vcross(nrm, vsub(a, c)), scale);
		const Vec3 g_k = vmul(vcross(nrm, vsub(b, a)), scale);
		const Vec3 grad = vadd(vadd(vmul(g_i, phi[i]), vmul(g_j, phi[j])),
		                      vmul(g_k, phi[k]));
		const double glen = vlen(grad);
		if (glen < 1e-12) {
			currentTri = -1;
			break;
		}

		// Project -grad onto triangle plane and normalize
		Vec3 nn = vcross(vsub(b, a), vsub(c, a));
		double nnlen = vlen(nn);
		Vec3 d = vmul(grad, -1.0);
		if (nnlen > 1e-12) {
			Vec3 nunit = vmul(nn, 1.0 / nnlen);
			d = vsub(d, vmul(nunit, vdot(d, nunit)));
		}
		double dlen = vlen(d);
		if (dlen < 1e-12) {
			currentTri = -1;
			break;
		}
		d = vmul(d, 1.0 / dlen);

		// Try a step of size stepSize
		Vec3 target = vadd(pos, vmul(d, stepSize));
		double wa, wb, wc;
		barycentric(target, a, b, c, wa, wb, wc);

		bool inside = (wa >= -1e-8 && wb >= -1e-8 && wc >= -1e-8);
		if (inside) {
			pos = target;
			pathPts.push_back(pos);
			steps++;
			continue;
		}

		// We're leaving the triangle. Find exact exit.
		int exitEdge;
		double te;
		if (!findTriangleExit(pos, a, b, c, d, exitEdge, te)) {
			// Gradient tracing failed here; break and let fallback handle it
			currentTri = -1;
			break;
		}

		// Don't overshoot our intended step size
		if (te > stepSize) {
			pos = vadd(pos, vmul(d, stepSize));
			pathPts.push_back(pos);
			steps++;
			continue;
		}

		// Move to the exit point on the edge
		pos = vadd(pos, vmul(d, te));
		pathPts.push_back(pos);

		// Determine which edge we crossed
		int e0, e1;
		if (exitEdge == 0) {
			e0 = j;
			e1 = k;
		} else if (exitEdge == 1) {
			e0 = i;
			e1 = k;
		} else {
			e0 = i;
			e1 = j;
		}

		EdgeKey ek{std::min(e0, e1), std::max(e0, e1)};
		auto it = edgeFaces.find(ek);
		if (it == edgeFaces.end()) {
			currentTri = -1;
			break;
		}
		int nextTri = -1;
		for (int nf : it->second) {
			if (nf != currentTri) {
				nextTri = nf;
				break;
			}
		}
		if (nextTri < 0) {
			currentTri = -1;
			break;
		}

		currentTri = nextTri;
		steps++;
	}

	// If smooth tracing failed or didn't reach close to source, append source
	pathPts.push_back(verts[startId]);

	// ------------------------------------------------------------------
	// 7. Fallback: if smooth tracing produced too few points, run greedy
	//    vertex descent and blend with the smooth trace.
	// ------------------------------------------------------------------
	if (pathPts.size() <= 3 || currentTri < 0) {
		std::vector<int> vpath;
		vpath.push_back(endId);
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
				for (int nb : neighbors[current]) {
					if (!visited[nb] && phi[nb] < bestVal + 1e-6) {
						bestVal = phi[nb];
						best = nb;
					}
				}
			}
			if (best == -1)
				break;
			vpath.push_back(best);
			current = best;
			visited[current] = 1;
		}

		bool reachedSource = (vpath.back() == startId);
		if (!reachedSource) {
			// Final fallback: Dijkstra on edge lengths
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
			if (parent[endId] != -1 || startId == endId) {
				vpath.clear();
				for (int v = endId; v != -1; v = parent[v]) {
					vpath.push_back(v);
				}
				std::reverse(vpath.begin(), vpath.end());
				reachedSource = true;
			}
		}

		if (!reachedSource) {
			return c;
		}

		// Replace pathPts with vertex path + subdivision for smoothness
		pathPts.clear();
		for (size_t vi = 0; vi < vpath.size(); ++vi) {
			pathPts.push_back(verts[vpath[vi]]);
			if (vi + 1 < vpath.size()) {
				// Add one midpoint for visual smoothness
				Vec3 mid = vadd(verts[vpath[vi]], verts[vpath[vi + 1]]);
				mid = vmul(mid, 0.5);
				pathPts.push_back(mid);
			}
		}
	}

	// Remove duplicate consecutive points
	std::vector<Vec3> deduped;
	deduped.reserve(pathPts.size());
	for (const Vec3 &p : pathPts) {
		if (deduped.empty() || vdist(p, deduped.back()) > 1e-9) {
			deduped.push_back(p);
		}
	}
	c.points = std::move(deduped);

	for (size_t i = 1; i < c.points.size(); i++) {
		c.length += vlen(vsub(c.points[i], c.points[i - 1]));
	}
	return c;
}
