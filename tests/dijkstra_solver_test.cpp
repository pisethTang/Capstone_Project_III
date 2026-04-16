// Unit tests for shortest-path solver behavior on small synthetic meshes.

#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <utility>
#include <vector>

#include "geodesic_lab/algorithms/dijkstra_solver.hpp"
#include "geodesic_lab/mesh/adjacency_builder.hpp"
#include "geodesic_lab/mesh/mesh.hpp"

namespace {

Mesh makeMesh(const std::vector<Vec3> &vertices,
		      const std::vector<std::pair<int, int>> &edges) {
	Mesh mesh;
	mesh.vertices = vertices;
	mesh.graph.resize(vertices.size());
	for (const auto &edge : edges) {
		addUndirectedEdge(mesh, edge.first, edge.second);
	}
	return mesh;
}

} // namespace



// Tests for Dijkstra solver behavior on small synthetic meshes.
TEST(DijkstraSolverTest, StartEqualsTargetReturnsZeroDistanceAndSingleNodePath) {
	const Mesh mesh = makeMesh(
		{{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}}, 
		{{0, 1}}
	);
	// same starting point
	int startVertex = 0;
	int endVertex = 0;
	const DijkstraResult result = solveDijkstra(mesh, startVertex, endVertex);

	ASSERT_TRUE(result.reachable);
	EXPECT_DOUBLE_EQ(result.totalDistance, 0.0);
	ASSERT_EQ(result.path.size(), 1u);
	EXPECT_EQ(result.path[0], 0);
	ASSERT_EQ(result.allDistances.size(), 2u);
	EXPECT_DOUBLE_EQ(result.allDistances[0], 0.0);
}

// Tests that when the target vertex is unreachable from the start, the result indicates unreachable and the path is empty.
TEST(DijkstraSolverTest, DisconnectedGraphReturnsUnreachableWithEmptyPath) {
	const Mesh mesh =
	    makeMesh({{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {5.0, 0.0, 0.0}}, {{0, 1}});

	const DijkstraResult result = solveDijkstra(mesh, 0, 2);

	EXPECT_FALSE(result.reachable);
	EXPECT_TRUE(result.path.empty());
	EXPECT_GE(result.totalDistance,
		  std::numeric_limits<double>::max() / 2.0);
	ASSERT_EQ(result.allDistances.size(), 3u);
	EXPECT_GE(result.allDistances[2],
		  std::numeric_limits<double>::max() / 2.0);
}


// Tests that when multiple paths exist, the solver chooses the one with the lowest total weight.
TEST(DijkstraSolverTest, ChoosesLowerWeightRouteOverAlternativePath) {
	const Mesh mesh = makeMesh(
	    {
		{0.0, 0.0, 0.0},
		{1.0, 0.0, 0.0},
		{0.0, 3.0, 0.0},
		{2.0, 0.0, 0.0},
	    },
	    {
		{0, 1},
		{1, 3},
		{0, 2},
		{2, 3},
	    });

	const DijkstraResult result = solveDijkstra(mesh, 0, 3);

	ASSERT_TRUE(result.reachable);
	const std::vector<int> expectedPath{0, 1, 3};
	EXPECT_EQ(result.path, expectedPath);
	EXPECT_NEAR(result.totalDistance, 2.0, 1e-12);
}


// Tests that the allDistances vector in the result has the same size as the number of vertices and contains finite values for reachable vertices.
TEST(DijkstraSolverTest, AllDistancesVectorMatchesVertexCountAndIsFiniteForReachable) {
	const Mesh mesh = makeMesh(
	    {
		{0.0, 0.0, 0.0},
		{1.0, 0.0, 0.0},
		{0.0, 3.0, 0.0},
		{2.0, 0.0, 0.0},
	    },
	    {
		{0, 1},
		{1, 3},
		{0, 2},
		{2, 3},
	    });

	const DijkstraResult result = solveDijkstra(mesh, 0, 3);

	ASSERT_EQ(result.allDistances.size(), mesh.vertices.size());
	for (const double d : result.allDistances) {
		EXPECT_TRUE(std::isfinite(d));
	}
}
