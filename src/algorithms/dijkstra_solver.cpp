#include "../../include/geodesic_lab/algorithms/dijkstra_solver.hpp"

#include <algorithm>
#include <limits>
#include <queue>
#include <vector>

DijkstraResult solveDijkstra(const Mesh &mesh, int start, int target) {
	const int n = static_cast<int>(mesh.vertices.size());
	std::vector<double> min_dist(n, std::numeric_limits<double>::max());
	std::vector<int> parent(n, -1);

	min_dist[start] = 0;
	std::priority_queue<std::pair<double, int>, std::vector<std::pair<double, int>>,
	                    std::greater<>>
	    pq;
	pq.push({0, start});

	while (!pq.empty()) {
		double d = pq.top().first;
		int u = pq.top().second;
		pq.pop();

		if (u == target)
			break;
		if (d > min_dist[u])
			continue;

		for (const auto &edge : mesh.graph[u]) {
			if (min_dist[u] + edge.weight < min_dist[edge.targetVertex]) {
				min_dist[edge.targetVertex] = min_dist[u] + edge.weight;
				parent[edge.targetVertex] = u;
				pq.push({min_dist[edge.targetVertex], edge.targetVertex});
			}
		}
	}

	std::vector<int> path;
	const bool reachable = (start == target) || (parent[target] != -1);
	if (reachable) {
		for (int v = target; v != -1; v = parent[v]) {
			path.push_back(v);
		}
		std::reverse(path.begin(), path.end());
	}

	return {min_dist[target], reachable, path, min_dist};
}
