#pragma once

#include "OgivaError.h"
#include "OgivaIntegrator.h"
#include "OgivaPointMass.h"
#include "OgivaState.h"
#include "OgivaVector.h"

#include <cstdint>

namespace Ogiva
{
	struct FPlane
	{
		FVector3 Normal;
		double Offset = 0.0;
	};

	enum class EStepControl : std::uint8_t
	{
		Fixed,
		Adaptive,
	};

	struct FFlightSettings
	{
		EStepControl StepControl = EStepControl::Fixed;
		double TimeStep = 0.0;
		double MaxTime = 0.0;
		double AbsoluteTolerance = 0.0;
		double RelativeTolerance = 0.0;
	};

	struct FPlaneCrossing
	{
		double Time = 0.0;
		FProjectileState State;
	};

	[[nodiscard]] OGIVACORE_API EError FlyToPlane(const IIntegrator& Integrator, const FPointMassModel& Model,
												  const FProjectileState& Initial, const FPlane& Plane,
												  const FFlightSettings& Settings, FPlaneCrossing& OutCrossing);
}
