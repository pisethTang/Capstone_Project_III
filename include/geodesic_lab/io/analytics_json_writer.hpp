#pragma once

#include <string>

#include "../analytics/analytic_types.hpp"

void writeAnalyticsJSON(const std::string &outputFilename,
						const std::string &outputPath,
						const AnalyticsResult &res);
