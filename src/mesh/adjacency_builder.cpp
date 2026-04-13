#include "../../include/geodesic_lab/mesh/adjacency_builder.hpp"

#include <algorithm>
#include <cmath>

namespace {

double edgeDistance(const Vec3 &v1, const Vec3 &v2) {
	const double dx = v2.x - v1.x;
	const double dy = v2.y - v1.y;
	const double dz = v2.z - v1.z;
	return std::sqrt(dx * dx + dy * dy + dz * dz);
}

} // namespace

void addUndirectedEdge(Mesh &mesh, int v1_idx, int v2_idx) {
	const int maxIndex = std::max(v1_idx, v2_idx);
	if (maxIndex >= 0 && mesh.graph.size() <= static_cast<size_t>(maxIndex)) {
		mesh.graph.resize(static_cast<size_t>(maxIndex) + 1);
	}
	const double d = edgeDistance(mesh.vertices[v1_idx], mesh.vertices[v2_idx]);
	mesh.graph[v1_idx].push_back({v2_idx, d});
	mesh.graph[v2_idx].push_back({v1_idx, d});
}
