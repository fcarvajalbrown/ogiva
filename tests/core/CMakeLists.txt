add_library(Catch2 STATIC
	vendor/catch2/catch_amalgamated.cpp)

target_include_directories(Catch2 SYSTEM PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/vendor/catch2)

target_compile_features(Catch2 PUBLIC cxx_std_20)

add_executable(OgivaCoreTests
	ErrorTests.cpp
	VersionTests.cpp)

target_link_libraries(OgivaCoreTests PRIVATE Ogiva::Core Catch2)

target_compile_definitions(OgivaCoreTests PRIVATE
	OGIVA_EXPECTED_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
	OGIVA_EXPECTED_VERSION_MINOR=${PROJECT_VERSION_MINOR}
	OGIVA_EXPECTED_VERSION_PATCH=${PROJECT_VERSION_PATCH})

if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
	target_compile_options(OgivaCoreTests PRIVATE /W4 /permissive- /utf-8)
else()
	target_compile_options(OgivaCoreTests PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(OgivaCoreTests PROPERTIES COMPILE_WARNING_AS_ERROR ON)

include(${CMAKE_CURRENT_SOURCE_DIR}/vendor/catch2/Catch.cmake)
catch_discover_tests(OgivaCoreTests)
