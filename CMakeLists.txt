cmake_minimum_required(VERSION 3.25)

set(OGIVA_UPLUGIN ${CMAKE_CURRENT_SOURCE_DIR}/plugin/Ogiva/Ogiva.uplugin)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${OGIVA_UPLUGIN})
file(READ ${OGIVA_UPLUGIN} OGIVA_UPLUGIN_JSON)
string(JSON OGIVA_VERSION GET "${OGIVA_UPLUGIN_JSON}" VersionName)

project(Ogiva
	VERSION ${OGIVA_VERSION}
	DESCRIPTION "Ballistics and shooting-analytics core for Unreal Engine"
	LANGUAGES CXX)

option(OGIVA_BUILD_TESTS "Build the OgivaCore Catch2 test suite" ON)

add_subdirectory(plugin/Ogiva/Source/OgivaCore)

if(OGIVA_BUILD_TESTS)
	enable_testing()
	add_subdirectory(tests/core)
endif()
