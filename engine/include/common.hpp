#pragma once

#include <array>
#include <string>

// Shared types/helpers used by multiple compilation units.

struct Vec3 {
	double x;
	double y;
	double z;
};

using Face = std::array<int, 3>;

static inline std::string jsonEscape(const std::string &s) {
	std::string out;
	out.reserve(s.size() + 8);
	for (char c : s) {
		switch (c) {
		case '\\':
			out += "\\\\";
			break;
		case '"':
			out += "\\\"";
			break;
		case '\n':
			out += "\\n";
			break;
		case '\r':
			out += "\\r";
			break;
		case '\t':
			out += "\\t";
			break;
		default:
			out += c;
			break;
		}
	}
	return out;
}
