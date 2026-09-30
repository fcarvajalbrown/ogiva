#pragma once

#include "OgivaError.h"
#include "OgivaFlight.h"
#include "OgivaIntegrator.h"
#include "OgivaPointMass.h"
#include "OgivaVector.h"

namespace Ogiva
{
	struct FAimSolution
	{
		double Elevation = 0.0;
		double Azimuth = 0.0;
		FPlaneCrossing Crossing;
	};

	struct FZeroRequest
	{
		FVector3 Muzzle;
		double MuzzleSpeed = 0.0;
		double SightHeight = 0.0;
		double ZeroRange = 0.0;
	};

	struct FHold
	{
		double Drop = 0.0;
		double Drift = 0.0;
		double Elevation = 0.0;
		double Windage = 0.0;
		FPlaneCrossing Crossing;
	};

	[[nodiscard]] OGIVACORE_API FVector3 LaunchDirection(double Elevation, double Azimuth);

	[[nodiscard]] OGIVACORE_API EError SolveAim(const IIntegrator& Integrator, const FPointMassModel& Model,
												const FVector3& Muzzle, double MuzzleSpeed, const FVector3& Target,
												const FFlightSettings& Settings, FAimSolution& OutAim);

	[[nodiscard]] OGIVACORE_API EError SolveZero(const IIntegrator& Integrator, const FPointMassModel& Model,
												 const FZeroRequest& Request, const FFlightSettings& Settings,
												 FAimSolution& OutZero);

	[[nodiscard]] OGIVACORE_API EError SolveHold(const IIntegrator& Integrator, const FPointMassModel& Model,
												 const FZeroRequest& Request, const FAimSolution& Zero, double Range,
												 const FFlightSettings& Settings, FHold& OutHold);
}
