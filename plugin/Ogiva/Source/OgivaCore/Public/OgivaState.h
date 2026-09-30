#pragma once

#include "OgivaVector.h"

namespace Ogiva
{
	struct FProjectileState
	{
		FVector3 Position;
		FVector3 Velocity;

		[[nodiscard]] friend constexpr bool operator==(const FProjectileState&, const FProjectileState&) = default;
	};

	struct FStateDerivative
	{
		FVector3 Velocity;
		FVector3 Acceleration;
	};

	[[nodiscard]] constexpr FProjectileState Advance(const FProjectileState& State, const FStateDerivative& Derivative,
													 double TimeStep)
	{
		return {
			.Position = State.Position + Derivative.Velocity * TimeStep,
			.Velocity = State.Velocity + Derivative.Acceleration * TimeStep,
		};
	}
}
