// Unit tests for analytics dispatch and heat/analytic surface selection.

#include <gtest/gtest.h>

#include <cmath>
#include <string>
#include <vector>

#include "geodesic_lab/analytics/analytic_service.hpp"

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

} // namespace

TEST(AnalyticsServiceTest, NoVerticesReturnsExplicitError) {
	const AnalyticsResult out =
	    computeAnalyticsForModel("plane.obj", 0, 1, {}, {});

	EXPECT_EQ(out.error, "No vertices loaded from OBJ");
	EXPECT_TRUE(out.curves.empty());
}

TEST(AnalyticsServiceTest, OutOfRangeIndicesReturnExplicitError) {
	const std::vector<Vec3> verts = makeSquareVerts();

	const AnalyticsResult out =
	    computeAnalyticsForModel("plane.obj", 0, 99, verts, {});

	EXPECT_EQ(out.error, "startId/endId out of range");
	EXPECT_TRUE(out.curves.empty());
}

TEST(AnalyticsServiceTest, PlaneFileNameRoutesToPlaneAnalyticCurve) {
	const std::vector<Vec3> verts = {
	    {0.0, 0.0, 0.0},
	    {2.0, 0.0, 0.0},
	    {0.0, 2.0, 0.0},
	};

	const AnalyticsResult out =
	    computeAnalyticsForModel("/tmp/MY_PLANE.OBJ", 0, 1, verts, {});

	EXPECT_EQ(out.surfaceType, "plane");
	EXPECT_TRUE(out.error.empty());
	ASSERT_EQ(out.curves.size(), 1u);
	EXPECT_EQ(out.curves[0].name, "plane_straight_line");
	EXPECT_GT(out.curves[0].length, 0.0);
}

TEST(AnalyticsServiceTest, SphereFileNameRoutesToGreatCircleCurve) {
	const std::vector<Vec3> verts = {
	    {0.0, 0.0, 1.0},
	    {0.0, 1.0, 0.0},
	    {1.0, 0.0, 0.0},
	};

	const AnalyticsResult out =
	    computeAnalyticsForModel("sphere_low.obj", 0, 1, verts, {});

	EXPECT_EQ(out.surfaceType, "sphere");
	EXPECT_TRUE(out.error.empty());
	ASSERT_EQ(out.curves.size(), 1u);
	EXPECT_EQ(out.curves[0].name, "sphere_great_circle");
	EXPECT_TRUE(std::isfinite(out.curves[0].length));
	EXPECT_GT(out.curves[0].length, 0.0);
}

TEST(AnalyticsServiceTest, DonutOrTorusFileNameRoutesToTorusCurve) {
	const std::vector<Vec3> verts = {
	    {2.0, 0.0, 0.0},
	    {0.0, 2.0, 0.0},
	    {-2.0, 0.0, 0.0},
	    {0.0, -2.0, 0.0},
	    {2.5, 0.0, 0.5},
	    {0.0, 2.5, 0.5},
	    {-2.5, 0.0, -0.5},
	    {0.0, -2.5, -0.5},
	};

	const AnalyticsResult out =
	    computeAnalyticsForModel("donut.obj", 0, 2, verts, {});

	EXPECT_EQ(out.surfaceType, "torus");
	EXPECT_TRUE(out.error.empty());
	ASSERT_EQ(out.curves.size(), 1u);
	EXPECT_EQ(out.curves[0].name, "torus_geodesic");
	EXPECT_FALSE(out.curves[0].points.empty());
}

TEST(AnalyticsServiceTest, SaddleFileNameRoutesToSaddleCurve) {
	const std::vector<Vec3> verts = {
	    {-1.0, -1.0, 0.0},
	    {1.0, -1.0, 0.0},
	    {1.0, 1.0, 0.0},
	    {-1.0, 1.0, 0.0},
	    {0.5, 0.0, 0.25},
	    {0.0, 0.5, -0.25},
	};

	const AnalyticsResult out =
	    computeAnalyticsForModel("my_saddle.obj", 0, 2, verts, {});

	EXPECT_EQ(out.surfaceType, "saddle");
	EXPECT_TRUE(out.error.empty());
	ASSERT_EQ(out.curves.size(), 1u);
	EXPECT_EQ(out.curves[0].name, "saddle_geodesic");
	EXPECT_FALSE(out.curves[0].points.empty());
}

TEST(AnalyticsServiceTest, UnknownModelWithFacesUsesHeatMethod) {
	const std::vector<Vec3> verts = makeSquareVerts();
	const std::vector<Face> faces = makeSquareFaces();

	const AnalyticsResult out =
	    computeAnalyticsForModel("stanford-bunny.obj", 0, 2, verts, faces);

	EXPECT_EQ(out.surfaceType, "mesh");
	EXPECT_TRUE(out.error.empty());
	ASSERT_EQ(out.curves.size(), 1u);
	EXPECT_EQ(out.curves[0].name, "heat_geodesic");
	EXPECT_FALSE(out.curves[0].points.empty());
}

TEST(AnalyticsServiceTest, UnknownModelWithoutFacesReturnsUnsupportedError) {
	const std::vector<Vec3> verts = makeSquareVerts();

	const AnalyticsResult out =
	    computeAnalyticsForModel("custom_model.obj", 0, 1, verts, {});

	EXPECT_EQ(out.surfaceType, "unsupported");
	EXPECT_FALSE(out.error.empty());
	EXPECT_TRUE(out.curves.empty());
}

TEST(AnalyticsServiceTest, ComputeHeatForModelRequiresFaces) {
	const std::vector<Vec3> verts = makeSquareVerts();

	const AnalyticsResult out =
	    computeHeatForModel("mesh.obj", 0, 1, verts, {});

	EXPECT_EQ(out.surfaceType, "mesh");
	EXPECT_EQ(out.error, "No faces loaded from OBJ");
	EXPECT_TRUE(out.curves.empty());
}

TEST(AnalyticsServiceTest, ComputeHeatForModelReturnsCurveOnValidMesh) {
	const std::vector<Vec3> verts = makeSquareVerts();
	const std::vector<Face> faces = makeSquareFaces();

	const AnalyticsResult out =
	    computeHeatForModel("mesh.obj", 0, 2, verts, faces);

	EXPECT_EQ(out.surfaceType, "mesh");
	EXPECT_TRUE(out.error.empty());
	ASSERT_EQ(out.curves.size(), 1u);
	EXPECT_EQ(out.curves[0].name, "heat_geodesic");
	EXPECT_FALSE(out.curves[0].points.empty());
}
