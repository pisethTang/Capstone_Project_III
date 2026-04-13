// Unit tests for heat-method geodesic behavior on tiny synthetic meshes.

#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include "geodesic_lab/algorithms/heat_method.hpp"

namespace {

std::vector<Vec3> makeSquareVerts() {
	return {
	    {0.0, 0.0, 0.0},
	    {1.0, 0.0, 0.0},
	    {1.0, 1.0, 0.0},
	    {0.0, 1.0, 0.0},
	};
}

std::vector<Face> makeSquareFaces() {
	return {
	    Face{0, 1, 2},
	    Face{0, 2, 3},
	};
}

bool approxVec(const Vec3 &a, const Vec3 &b, double eps = 1e-9) {
	return std::fabs(a.x - b.x) <= eps && std::fabs(a.y - b.y) <= eps &&
	       std::fabs(a.z - b.z) <= eps;
}

} // namespace

TEST(HeatMethodTest, InvalidIndicesReturnEmptyCurve) {
	const std::vector<Vec3> verts = makeSquareVerts();
	const std::vector<Face> faces = makeSquareFaces();

	const AnalyticsCurve negativeStart =
	    makeHeatMethodGeodesic(verts, faces, -1, 2);
	const AnalyticsCurve outOfRangeEnd =
	    makeHeatMethodGeodesic(verts, faces, 0, 99);

	EXPECT_TRUE(negativeStart.points.empty());
	EXPECT_TRUE(outOfRangeEnd.points.empty());
}

TEST(HeatMethodTest, NoFacesReturnsEmptyCurve) {
	const std::vector<Vec3> verts = makeSquareVerts();
	const std::vector<Face> faces;

	const AnalyticsCurve curve = makeHeatMethodGeodesic(verts, faces, 0, 2);

	EXPECT_TRUE(curve.points.empty());
	EXPECT_DOUBLE_EQ(curve.length, 0.0);
}

TEST(HeatMethodTest, ValidMeshReturnsFinitePathBetweenEndpoints) {
	const std::vector<Vec3> verts = makeSquareVerts();
	const std::vector<Face> faces = makeSquareFaces();

	const AnalyticsCurve curve = makeHeatMethodGeodesic(verts, faces, 0, 2);

	ASSERT_FALSE(curve.points.empty());
	EXPECT_GE(curve.points.size(), 2u);
	EXPECT_TRUE(std::isfinite(curve.length));
	EXPECT_GE(curve.length, 0.0);
	EXPECT_TRUE(approxVec(curve.points.front(), verts[0]));
	EXPECT_TRUE(approxVec(curve.points.back(), verts[2]));
}

TEST(HeatMethodTest, StartEqualsEndReturnsSinglePointAndZeroLength) {
	const std::vector<Vec3> verts = makeSquareVerts();
	const std::vector<Face> faces = makeSquareFaces();

	const AnalyticsCurve curve = makeHeatMethodGeodesic(verts, faces, 1, 1);

	ASSERT_EQ(curve.points.size(), 1u);
	EXPECT_TRUE(approxVec(curve.points[0], verts[1]));
	EXPECT_DOUBLE_EQ(curve.length, 0.0);
}

TEST(HeatMethodTest, DisconnectedComponentsProduceNoPath) {
	const std::vector<Vec3> verts = {
	    {0.0, 0.0, 0.0},
	    {1.0, 0.0, 0.0},
	    {0.0, 1.0, 0.0},
	    {10.0, 0.0, 0.0},
	    {11.0, 0.0, 0.0},
	    {10.0, 1.0, 0.0},
	};
	const std::vector<Face> faces = {
	    Face{0, 1, 2},
	    Face{3, 4, 5},
	};

	const AnalyticsCurve curve = makeHeatMethodGeodesic(verts, faces, 0, 4);

	EXPECT_TRUE(curve.points.empty());
	EXPECT_DOUBLE_EQ(curve.length, 0.0);
}
