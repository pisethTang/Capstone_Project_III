cmake_minimum_required(VERSION 3.16)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(NOT DEFINED GEODESIC_EIGEN_DIR)
	if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../eigen/Eigen/Core")
		set(GEODESIC_EIGEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../eigen")
	elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../eigen/Eigen/Core")
		set(GEODESIC_EIGEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../eigen")
	elseif(EXISTS "/usr/include/eigen3/Eigen/Core")
		set(GEODESIC_EIGEN_DIR "/usr/include/eigen3")
	else()
		message(FATAL_ERROR
			"Eigen headers not found. Expected one of: ${CMAKE_CURRENT_SOURCE_DIR}/../../eigen, ${CMAKE_CURRENT_SOURCE_DIR}/../eigen, or /usr/include/eigen3")
	endif()
endif()

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
		${GEODESIC_EIGEN_DIR}
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