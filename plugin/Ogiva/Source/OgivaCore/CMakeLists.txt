add_library(OgivaCore STATIC
	Private/OgivaAim.cpp
	Private/OgivaDrag.cpp
	Private/OgivaError.cpp
	Private/OgivaFlight.cpp
	Private/OgivaIntegrator.cpp
	Private/OgivaPchip.cpp
	Private/OgivaPointMass.cpp
	Private/OgivaVersion.cpp)

add_library(Ogiva::Core ALIAS OgivaCore)

target_include_directories(OgivaCore PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/Public)

target_compile_features(OgivaCore PUBLIC cxx_std_20)

target_compile_definitions(OgivaCore
	PUBLIC
		OGIVACORE_API=
	PRIVATE
		OGIVA_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
		OGIVA_VERSION_MINOR=${PROJECT_VERSION_MINOR}
		OGIVA_VERSION_PATCH=${PROJECT_VERSION_PATCH})

if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
	target_compile_options(OgivaCore PUBLIC /fp:precise)
	if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		target_compile_options(OgivaCore PUBLIC /clang:-ffp-contract=off)
	endif()
	target_compile_options(OgivaCore PRIVATE /W4 /permissive- /utf-8)
else()
	target_compile_options(OgivaCore PUBLIC -ffp-contract=off -fno-fast-math)
	target_compile_options(OgivaCore PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wdouble-promotion)
endif()

set_target_properties(OgivaCore PROPERTIES COMPILE_WARNING_AS_ERROR ON)
