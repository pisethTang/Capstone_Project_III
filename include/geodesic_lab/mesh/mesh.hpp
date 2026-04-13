#pragma once

#include <vector>

#include "../types/mesh_types.hpp"
#include "../types/face.hpp"
#include "../types/vec3.hpp"

struct Mesh {
	std::vector<Vec3> vertices;
	std::vector<std::vector<Edge>> graph;
	std::vector<Face> faces;
};