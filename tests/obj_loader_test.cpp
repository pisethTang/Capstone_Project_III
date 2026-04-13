// Unit tests for OBJ ingestion and face/index parsing behavior.

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

#include "geodesic_lab/io/obj_loader.hpp"
#include "geodesic_lab/mesh/adjacency_builder.hpp"
#include "geodesic_lab/mesh/mesh.hpp"


namespace {

class TempObjFile {
  public:
	explicit TempObjFile(const std::string &contents) {
		const auto tmpDir = std::filesystem::temp_directory_path();
		path_ = tmpDir /
			("geodesic_lab_obj_loader_test_" + std::to_string(++counter_) +
			 ".obj");

		std::ofstream out(path_);
		out << contents;
	}

	~TempObjFile() {
		std::error_code ec;
		std::filesystem::remove(path_, ec);
	}

	const std::filesystem::path &path() const {
		return path_;
	}

  private:
	std::filesystem::path path_;
	static inline int counter_ = 0;
};

bool loadObj(const std::filesystem::path &path, Mesh &mesh) {
	return loadOBJIntoMesh(
	    path.string(), mesh,
	    [&](int a, int b) {
		addUndirectedEdge(mesh, a, b);
	    });
}

} // namespace

TEST(ObjLoaderTest, MissingFileReturnsFalse) {
	Mesh mesh;
	EXPECT_FALSE(loadOBJIntoMesh(
	    "/definitely/not/a/real/path/mesh.obj", mesh,
	    [&](int a, int b) {
		addUndirectedEdge(mesh, a, b);
	    }));
}

TEST(ObjLoaderTest, LoadsTriangleVerticesFaceAndAdjacency) {
	TempObjFile file(R"OBJ(
v 0 0 0
v 1 0 0
v 0 1 0
f 1 2 3
)OBJ");

	Mesh mesh;
	ASSERT_TRUE(loadObj(file.path(), mesh));

	ASSERT_EQ(mesh.vertices.size(), 3u);
	ASSERT_EQ(mesh.faces.size(), 1u);
	EXPECT_EQ(mesh.faces[0], (Face{0, 1, 2}));

	ASSERT_EQ(mesh.graph.size(), 3u);
	EXPECT_EQ(mesh.graph[0].size(), 2u);
	EXPECT_EQ(mesh.graph[1].size(), 2u);
	EXPECT_EQ(mesh.graph[2].size(), 2u);
}

TEST(ObjLoaderTest, TriangulatesQuadFaceAsFan) {
	TempObjFile file(R"OBJ(
v 0 0 0
v 1 0 0
v 1 1 0
v 0 1 0
f 1 2 3 4
)OBJ");

	Mesh mesh;
	ASSERT_TRUE(loadObj(file.path(), mesh));

	ASSERT_EQ(mesh.faces.size(), 2u);
	EXPECT_EQ(mesh.faces[0], (Face{0, 1, 2}));
	EXPECT_EQ(mesh.faces[1], (Face{0, 2, 3}));
}

TEST(ObjLoaderTest, ParsesSlashedAndNegativeIndices) {
	TempObjFile file(R"OBJ(
v 0 0 0
v 1 0 0
v 1 1 0
v 0 1 0
f 1/10/20 2/11/21 3/12/22
f -4 -3 -2
)OBJ");

	Mesh mesh;
	ASSERT_TRUE(loadObj(file.path(), mesh));

	ASSERT_EQ(mesh.faces.size(), 2u);
	EXPECT_EQ(mesh.faces[0], (Face{0, 1, 2}));
	EXPECT_EQ(mesh.faces[1], (Face{0, 1, 2}));
}

TEST(ObjLoaderTest, SkipsMalformedFacesWithoutCrashing) {
	TempObjFile file(R"OBJ(
v 0 0 0
v 1 0 0
v 0 1 0
f 1 2
f 1 2 99
f 1 2 nope
)OBJ");

	Mesh mesh;
	ASSERT_TRUE(loadObj(file.path(), mesh));

	EXPECT_EQ(mesh.vertices.size(), 3u);
	EXPECT_TRUE(mesh.faces.empty());
}
