cmake_minimum_required(VERSION 3.16)

find_package(GTest REQUIRED)

add_library(geodesic_lab_core STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/../src/algorithms/dijkstra_solver.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../src/algorithms/heat_method.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../src/analytics/analytic_service.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../src/analytics/analytic_surface_curves.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../src/analytics/analytic_normalization.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../src/analytics/analytic_string_utils.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../src/io/obj_loader.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../src/io/result_json_writer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../src/io/analytics_json_writer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../src/mesh/adjacency_builder.cpp
)

target_include_directories(geodesic_lab_core
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/../include
)

add_executable(cpp_unit_tests
	${CMAKE_CURRENT_SOURCE_DIR}/dijkstra_solver_test.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/obj_loader_test.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/heat_method_test.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/analytics_service_test.cpp
)

target_link_libraries(cpp_unit_tests
	PRIVATE
		geodesic_lab_core
		GTest::gtest
		GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(cpp_unit_tests)