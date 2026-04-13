#pragma once

#include "../mesh/mesh.hpp"
#include "dijkstra_result.hpp"

DijkstraResult solveDijkstra(const Mesh &mesh, int start, int target);
