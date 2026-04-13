#pragma once

#include <functional>
#include <string>

#include "../mesh/mesh.hpp"

using EdgeConsumer = std::function<void(int, int)>;

bool loadOBJIntoMesh(const std::string &filename, Mesh &mesh,
                     const EdgeConsumer &onEdge);

