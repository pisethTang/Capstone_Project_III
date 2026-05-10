#pragma once 


#include <string>

#include "geodesic_lab/mesh/adjacency_builder.hpp"
#include "geodesic_lab/mesh/mesh.hpp"

class MeshEngine {
  public:
	Mesh mesh;
    bool loadOBJ(const std::string &filename);
};