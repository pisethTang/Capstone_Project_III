
#include "geodesic_lab/mesh/mesh_engine.hpp"

#include "geodesic_lab/io/obj_loader.hpp"


bool MeshEngine::loadOBJ(const std::string &filename) {
    return loadOBJIntoMesh(
        filename, mesh, 
        [this](int v1_idx, int v2_idx) {
            addUndirectedEdge(mesh, v1_idx, v2_idx);
        }
    
    );
}