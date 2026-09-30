#include "OgivaVersion.h"

#include <catch_amalgamated.hpp>

TEST_CASE("GetVersion reports the Ogiva.uplugin VersionName", "[version]")
{
	constexpr Ogiva::FVersion Expected{
		.Major = OGIVA_EXPECTED_VERSION_MAJOR,
		.Minor = OGIVA_EXPECTED_VERSION_MINOR,
		.Patch = OGIVA_EXPECTED_VERSION_PATCH,
	};
	CHECK(Ogiva::GetVersion() == Expected);
}
