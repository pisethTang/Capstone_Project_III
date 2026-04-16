#include "../../include/geodesic_lab/io/obj_loader.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

bool loadOBJIntoMesh(const std::string &filename, Mesh &mesh,
                     const EdgeConsumer &onEdge) {
	std::ifstream file(filename);
	if (!file.is_open())
		return false;

	mesh.vertices.clear();
	mesh.graph.clear();
	mesh.faces.clear();

	std::string line;
	while (std::getline(file, line)) {
		std::istringstream iss(line);
		std::string type;
		iss >> type;
		if (type == "v") {
			Vec3 v;
			iss >> v.x >> v.y >> v.z;
			mesh.vertices.push_back(v);
		}
		else if (type == "f") {
			std::vector<std::string> tokens;
			std::string token;
			while (iss >> token) {
				tokens.push_back(token);
			}
			if (tokens.size() < 3)
				continue;

			auto toIndex = [&](const std::string &t) -> int {
				const auto slash = t.find('/');
				const std::string head =
				    (slash == std::string::npos) ? t : t.substr(0, slash);
				int idx = 0;
				try {
					idx = std::stoi(head);
				} catch (...) {
					return -1;
				}
				if (idx == 0)
					return -1;
				int resolved =
				    (idx > 0) ? (idx - 1)
				              : (static_cast<int>(mesh.vertices.size()) + idx);
				return resolved;
			};

			std::vector<int> faceIndices;
			faceIndices.reserve(tokens.size());
			for (const auto &t : tokens) {
				int resolved = toIndex(t);
				if (resolved < 0 || resolved >= (int)mesh.vertices.size()) {
					faceIndices.clear();
					break;
				}
				faceIndices.push_back(resolved);
			}
			if (faceIndices.size() < 3)
				continue;

			for (size_t i = 1; i + 1 < faceIndices.size(); i++) {
				const int a = faceIndices[0];
				const int b = faceIndices[i];
				const int c = faceIndices[i + 1];
				mesh.faces.push_back({a, b, c});
				onEdge(a, b);
				onEdge(b, c);
				onEdge(c, a);
			}
		}
	}

	return true;
}
