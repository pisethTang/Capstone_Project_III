#include "../../include/geodesic_lab/analytics/analytic_string_utils.hpp"

#include <string>

std::string basenameOnly(const std::string &path) {
	std::string s = path;
	for (auto &ch : s) {
		if (ch == '\\')
			ch = '/';
	}
	const auto slash = s.find_last_of('/');
	return (slash == std::string::npos) ? s : s.substr(slash + 1);
}

std::string lowerAscii(std::string s) {
	for (auto &c : s) {
		if (c >= 'A' && c <= 'Z')
			c = char(c - 'A' + 'a');
	}
	return s;
}