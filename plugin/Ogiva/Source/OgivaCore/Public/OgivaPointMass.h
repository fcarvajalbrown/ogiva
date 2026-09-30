#pragma once

#include "OgivaPchip.h"
#include "OgivaState.h"
#include "OgivaVector.h"

namespace Ogiva
{
	inline constexpr double StandardGravity = 9.80665;
	inline constexpr double EarthRotationRate = 7.292115e-5;

	struct FPointMassParams
	{
		const FPchipCurve* DragCurve = nullptr;
		double BallisticCoefficient = 0.0;
		double AirDensity = 0.0;
		double SpeedOfSound = 0.0;
		FVector3 Wind;
		FVector3 Gravity;
		FVector3 EarthRotation;
	};

	class FPointMassModel
	{
	public:
		OGIVACORE_API explicit FPointMassModel(const FPointMassParams& InParams);

		[[nodiscard]] OGIVACORE_API FStateDerivative Derivative(double Time, const FProjectileState& State) const;

	private:
		FPointMassParams Params;
	};

	[[nodiscard]] OGIVACORE_API double BallisticCoefficientFromImperial(double PoundsPerSquareInch);
	[[nodiscard]] OGIVACORE_API FVector3 EarthRotationInFrame(double LatitudeRad, double AzimuthRad);
}
