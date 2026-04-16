cmake_minimum_required(VERSION 3.16)

project(geodesic_lab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(GEODESIC_ENGINE_SOURCES
	src/algorithms/dijkstra_solver.cpp
	src/algorithms/heat_method.cpp
	src/analytics/analytic_service.cpp
	src/analytics/analytic_surface_curves.cpp
	src/analytics/analytic_normalization.cpp
	src/analytics/analytic_string_utils.cpp
	src/io/analytics_json_writer.cpp
	src/io/obj_loader.cpp
	src/io/result_json_writer.cpp
	src/mesh/adjacency_builder.cpp
)

add_executable(geodesic_engine
	main.cpp
	${GEODESIC_ENGINE_SOURCES}
)

set_target_properties(geodesic_engine PROPERTIES
	OUTPUT_NAME main
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

include(CTest)
if(BUILD_TESTING)
	enable_testing()
endif()

if(BUILD_TESTING)
	add_subdirectory(tests)
endif()