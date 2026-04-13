#pragma once

struct Christoffel2 {
	// Gamma^u_{uu}, Gamma^u_{uv}, Gamma^u_{vv}, Gamma^v_{uu}, Gamma^v_{uv},
	// Gamma^v_{vv}
	double gu_uu = 0.0;
	double gu_uv = 0.0;
	double gu_vv = 0.0;
	double gv_uu = 0.0;
	double gv_uv = 0.0;
	double gv_vv = 0.0;
};