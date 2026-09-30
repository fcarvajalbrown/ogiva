#include "OgivaError.h"

#include <catch_amalgamated.hpp>

#include <string_view>

TEST_CASE("ToString names every error code", "[error]")
{
	using Ogiva::EError;
	CHECK(Ogiva::ToString(EError::None) == std::string_view{"None"});
	CHECK(Ogiva::ToString(EError::InvalidArgument) == std::string_view{"InvalidArgument"});
	CHECK(Ogiva::ToString(EError::OutOfRange) == std::string_view{"OutOfRange"});
	CHECK(Ogiva::ToString(EError::NotConverged) == std::string_view{"NotConverged"});
	CHECK(Ogiva::ToString(EError::TargetNotReached) == std::string_view{"TargetNotReached"});
}

TEST_CASE("ToString falls back for values outside the enum", "[error]")
{
	CHECK(Ogiva::ToString(static_cast<Ogiva::EError>(255)) == std::string_view{"Unknown"});
}
