#pragma once

#include "OgivaAim.h"

namespace Ogiva::AimFallback
{
	[[nodiscard]] EError SolveByBracketing(const IIntegrator& Integrator, const FPointMassModel& Model,
										   const FVector3& Muzzle, double MuzzleSpeed, const FVector3& Target,
										   const FFlightSettings& Settings, FAimSolution& OutAim);
}
