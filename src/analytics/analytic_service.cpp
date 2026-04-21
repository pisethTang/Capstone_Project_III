#include "../../include/geodesic_lab/analytics/analytic_service.hpp"
#include "../../include/geodesic_lab/analytics/analytic_curves.hpp"
#include "../../include/geodesic_lab/algorithms/heat_method.hpp"

#include <cmath>
#include <sstream>

AnalyticsResult computeAnalyticsForModel(const std::string &inputFileName,
                                         int startId, int endId,
                                         const std::vector<Vec3> &objVertices,
                                         const std::vector<Face> &faces) {
	// Output struct to fill in and return
	AnalyticsResult out;
	out.inputFileName = inputFileName;
	out.startId = startId;
	out.endId = endId;

	// Basic validation of inputs
	if (objVertices.empty()) {
		out.error = "No vertices loaded from OBJ";
		return out;
	}
	if (startId < 0 || endId < 0 || startId >= (int)objVertices.size() ||
	    endId >= (int)objVertices.size()) {
		out.error = "startId/endId out of range";
		return out;
	}



	// Normalize the model to be centered at the origin and fit within a unit sphere.
	const ObjNormalizeTransform t = computeNormalizeTransform(objVertices);
	const Vec3 p1 = applyNormalize(t, objVertices[(size_t)startId]);
	const Vec3 p2 = applyNormalize(t, objVertices[(size_t)endId]);
	const double lengthScale = (t.scale > 1e-12) ? (1.0 / t.scale) : 1.0;

	// Precompute the normalized vertices for potential use in analytic curve generation or heat method.
	std::vector<Vec3> normalizedVerts;
	normalizedVerts.reserve(objVertices.size());
	for (const auto &v : objVertices) {
		normalizedVerts.push_back(applyNormalize(t, v));
	}

	// 
	const std::string name = lowerAscii(basenameOnly(inputFileName));

	// Option 1: infer the analytic surface type from the OBJ name.
	// Only a couple of "simple surfaces" for now.
	if (name.find("plane") != std::string::npos) {
		out.surfaceType = "plane";
		out.curves.push_back(makePlaneGeodesic(p1, p2, 64));
		if (!out.curves.empty()) {
			out.curves.back().length *= lengthScale;
		}
		{
			std::ostringstream oss;
			oss << "{\"type\":\"plane\"}";
			out.surfaceParams = oss.str();
		}
		return out;
	} else if (name.find("sphere") != std::string::npos) {
		out.surfaceType = "sphere";
		out.curves.push_back(makeSphereGreatCircle(p1, p2, 128));
		if (!out.curves.empty()) {
			out.curves.back().length *= lengthScale;
		}
		const double r1 = std::sqrt(p1.x*p1.x + p1.y*p1.y + p1.z*p1.z);
		const double r2 = std::sqrt(p2.x*p2.x + p2.y*p2.y + p2.z*p2.z);
		const double r = (r1 > 1e-12 && r2 > 1e-12) ? (0.5 * (r1 + r2)) : std::max(r1, r2);
		{
			std::ostringstream oss;
			oss << "{\"center\":[0,0,0],\"radius\":" << r << "}";
			out.surfaceParams = oss.str();
		}
		return out;
	} else if (name.find("torus") != std::string::npos ||
	    name.find("donut") != std::string::npos) {
		out.surfaceType = "torus";
		const TorusParams torus = estimateTorusParams(normalizedVerts);
		out.curves.push_back(makeTorusApproxGeodesic(p1, p2, torus, 160));
		if (!out.curves.empty()) {
			out.curves.back().length *= lengthScale;
		}
		{
			std::ostringstream oss;
			oss << "{\"center\":[" << torus.center.x << "," << torus.center.y << "," << torus.center.z << "],"
			    << "\"majorRadius\":" << torus.majorRadius << ","
			    << "\"minorRadius\":" << torus.minorRadius << "}";
			out.surfaceParams = oss.str();
		}
		return out;
	} else if (name.find("saddle") != std::string::npos) {
		out.surfaceType = "saddle";
		const SaddleParams saddle = estimateSaddleParams(normalizedVerts);
		out.curves.push_back(makeSaddleApproxGeodesic(p1, p2, saddle, 160));
		if (!out.curves.empty()) {
			out.curves.back().length *= lengthScale;
		}
		{
			std::ostringstream oss;
			oss << "{\"center\":[" << saddle.center.x << "," << saddle.center.y << "," << saddle.center.z << "],"
			    << "\"a\":" << saddle.a << "}";
			out.surfaceParams = oss.str();
		}
		return out;
	}

	// Heat method for any mesh.
	if (!faces.empty()) {
		out.surfaceType = "mesh";
		AnalyticsCurve heat = makeHeatMethodGeodesic(normalizedVerts, faces, startId, endId);
		if (!heat.points.empty()) {
			heat.length *= lengthScale;
			out.curves.push_back(std::move(heat));
			return out;
		}

		out.error = "Heat method failed to produce a path";
		return out;
	}

	out.surfaceType = "unsupported";
	out.error = "Analytics currently supports plane.obj, sphere.obj, "
	            "donut.obj, saddle.obj, or heat method on triangle meshes";
	return out;
}

AnalyticsResult computeHeatForModel(const std::string &inputFileName,
                                    int startId, int endId,
                                    const std::vector<Vec3> &objVertices,
                                    const std::vector<Face> &faces) {
	AnalyticsResult out;
	out.inputFileName = inputFileName;
	out.startId = startId;
	out.endId = endId;
	out.surfaceType = "mesh";

	if (objVertices.empty()) {
		out.error = "No vertices loaded from OBJ";
		return out;
	}
	if (faces.empty()) {
		out.error = "No faces loaded from OBJ";
		return out;
	}
	if (startId < 0 || endId < 0 || startId >= (int)objVertices.size() ||
	    endId >= (int)objVertices.size()) {
		out.error = "startId/endId out of range";
		return out;
	}

	const ObjNormalizeTransform t = computeNormalizeTransform(objVertices);
	std::vector<Vec3> normalizedVerts;
	normalizedVerts.reserve(objVertices.size());
	for (const auto &v : objVertices) {
		normalizedVerts.push_back(applyNormalize(t, v));
	}
	const double lengthScale = (t.scale > 1e-12) ? (1.0 / t.scale) : 1.0;

	AnalyticsCurve heat =
	    makeHeatMethodGeodesic(normalizedVerts, faces, startId, endId);
	if (heat.points.empty()) {
		out.error = "Heat method failed to produce a path";
		return out;
	}
	heat.length *= lengthScale;
	out.curves.push_back(std::move(heat));
	return out;
}
