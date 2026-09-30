#include "OgivaPointMass.h"

#include <cmath>
#include <limits>
#include <numbers>

namespace Ogiva
{
	namespace
	{
		constexpr double KilogramsPerPound = 0.45359237;
		constexpr double MetresPerInch = 0.0254;
	}

	FPointMassModel::FPointMassModel(const FPointMassParams& InParams)
		: Params(InParams)
	{
	}

	FStateDerivative FPointMassModel::Derivative([[maybe_unused]] double Time, const FProjectileState& State) const
	{
		const FVector3 AirVelocity = State.Velocity - Params.Wind;
		const double AirSpeed = Length(AirVelocity);
		const double DragCoefficient = Params.DragCurve != nullptr
										   ? Params.DragCurve->Evaluate(AirSpeed / Params.SpeedOfSound)
										   : std::numeric_limits<double>::quiet_NaN();
		const double DragScale =
			std::numbers::pi / 8.0 * Params.AirDensity * DragCoefficient * AirSpeed / Params.BallisticCoefficient;

		return {
			.Velocity = State.Velocity,
			.Acceleration =
				Params.Gravity - 2.0 * Cross(Params.EarthRotation, State.Velocity) - AirVelocity * DragScale,
		};
	}

	double BallisticCoefficientFromImperial(double PoundsPerSquareInch)
	{
		return PoundsPerSquareInch * KilogramsPerPound / (MetresPerInch * MetresPerInch);
	}

	FVector3 EarthRotationInFrame(double LatitudeRad, double AzimuthRad)
	{
		const double Horizontal = EarthRotationRate * std::cos(LatitudeRad);
		return {
			.X = Horizontal * std::cos(AzimuthRad),
			.Y = Horizontal * std::sin(AzimuthRad),
			.Z = EarthRotationRate * std::sin(LatitudeRad),
		};
	}
}
