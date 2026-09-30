cmake_minimum_required(VERSION 3.25)

project(Ogiva
	VERSION 0.1.0
	DESCRIPTION "Ballistics and shooting-analytics core for Unreal Engine"
	LANGUAGES CXX)

option(OGIVA_BUILD_TESTS "Build the OgivaCore Catch2 test suite" ON)

add_subdirectory(plugin/Ogiva/Source/OgivaCore)

if(OGIVA_BUILD_TESTS)
	enable_testing()
	add_subdirectory(tests/core)
endif()
