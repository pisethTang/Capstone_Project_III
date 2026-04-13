#pragma once

#include "analytic_params.hpp"
#include "analytic_types.hpp"

AnalyticsCurve makePlaneGeodesic(const Vec3 &p1, const Vec3 &p2, int samples);

AnalyticsCurve makeSphereGreatCircle(const Vec3 &p1, const Vec3 &p2,
                                     int samples);

AnalyticsCurve makeTorusApproxGeodesic(const Vec3 &p1, const Vec3 &p2,
                                       const TorusParams &torus,
                                       int samples);

AnalyticsCurve makeSaddleApproxGeodesic(const Vec3 &p1, const Vec3 &p2,
                                        const SaddleParams &saddle,
                                        int samples);