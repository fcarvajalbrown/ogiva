#include "OgivaState.h"

#include <catch_amalgamated.hpp>

using Ogiva::FProjectileState;
using Ogiva::FStateDerivative;
using Ogiva::FVector3;

TEST_CASE("Advance applies one explicit step to position and velocity", "[state]")
{
	constexpr FProjectileState State{
		.Position = {.X = 0.0, .Y = 0.0, .Z = 1.5},
		.Velocity = {.X = 800.0, .Y = 0.0, .Z = 0.0},
	};
	constexpr FStateDerivative Derivative{
		.Velocity = State.Velocity,
		.Acceleration = {.X = -400.0, .Y = 0.0, .Z = -9.75},
	};

	constexpr FProjectileState Next = Ogiva::Advance(State, Derivative, 0.5);

	STATIC_CHECK(Next.Position == FVector3{.X = 400.0, .Y = 0.0, .Z = 1.5});
	STATIC_CHECK(Next.Velocity == FVector3{.X = 600.0, .Y = 0.0, .Z = -4.875});
}

TEST_CASE("Advance with a zero step returns the same state", "[state]")
{
	constexpr FProjectileState State{
		.Position = {.X = 1.0, .Y = 2.0, .Z = 3.0},
		.Velocity = {.X = 4.0, .Y = 5.0, .Z = 6.0},
	};
	constexpr FStateDerivative Derivative{
		.Velocity = State.Velocity,
		.Acceleration = {.X = 7.0, .Y = 8.0, .Z = 9.0},
	};

	STATIC_CHECK(Ogiva::Advance(State, Derivative, 0.0) == State);
}
