#include "OgivaVector.h"

#include <catch_amalgamated.hpp>

using Ogiva::FVector3;

TEST_CASE("Vector arithmetic is componentwise", "[vector]")
{
	constexpr FVector3 A{.X = 1.0, .Y = 2.0, .Z = 3.0};
	constexpr FVector3 B{.X = 4.0, .Y = -5.0, .Z = 6.0};

	STATIC_CHECK(A + B == FVector3{.X = 5.0, .Y = -3.0, .Z = 9.0});
	STATIC_CHECK(A - B == FVector3{.X = -3.0, .Y = 7.0, .Z = -3.0});
	STATIC_CHECK(-A == FVector3{.X = -1.0, .Y = -2.0, .Z = -3.0});
	STATIC_CHECK(A * 2.0 == FVector3{.X = 2.0, .Y = 4.0, .Z = 6.0});
	STATIC_CHECK(2.0 * A == A * 2.0);
	STATIC_CHECK(B / 2.0 == FVector3{.X = 2.0, .Y = -2.5, .Z = 3.0});
	STATIC_CHECK(FVector3::Zero() == FVector3{});
}

TEST_CASE("Dot and cross products", "[vector]")
{
	constexpr FVector3 A{.X = 1.0, .Y = 2.0, .Z = 3.0};
	constexpr FVector3 B{.X = 4.0, .Y = -5.0, .Z = 6.0};

	STATIC_CHECK(Ogiva::Dot(A, B) == 12.0);
	STATIC_CHECK(Ogiva::Cross(A, B) == FVector3{.X = 27.0, .Y = 6.0, .Z = -13.0});
	STATIC_CHECK(Ogiva::Dot(Ogiva::Cross(A, B), A) == 0.0);
}

TEST_CASE("Frame is right-handed and Z-up", "[vector]")
{
	constexpr FVector3 UnitX{.X = 1.0};
	constexpr FVector3 UnitY{.Y = 1.0};

	STATIC_CHECK(Ogiva::Cross(UnitX, UnitY) == FVector3::Up());
}

TEST_CASE("Length of a 3-4-12 vector is exactly 13", "[vector]")
{
	constexpr FVector3 V{.X = 3.0, .Y = 4.0, .Z = 12.0};

	STATIC_CHECK(Ogiva::SquaredLength(V) == 169.0);
	CHECK(Ogiva::Length(V) == 13.0);
}
