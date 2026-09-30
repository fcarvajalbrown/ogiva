#pragma once

#include "OgivaError.h"
#include "OgivaPointMass.h"
#include "OgivaState.h"

#include <span>

namespace Ogiva
{
	struct FStepResult
	{
		FProjectileState State;
		FStateDerivative EndDerivative;
		FProjectileState LocalError;
	};

	class OGIVACORE_API IIntegrator
	{
	public:
		IIntegrator() = default;
		IIntegrator(const IIntegrator&) = default;
		IIntegrator(IIntegrator&&) = default;
		IIntegrator& operator=(const IIntegrator&) = default;
		IIntegrator& operator=(IIntegrator&&) = default;
		virtual ~IIntegrator() = default;

		[[nodiscard]] virtual EError StepBatch(const FPointMassModel& Model, double Time,
											   std::span<const FProjectileState> States, double TimeStep,
											   std::span<FStepResult> OutResults) const = 0;
	};

	class OGIVACORE_API FEulerIntegrator final : public IIntegrator
	{
	public:
		[[nodiscard]] EError StepBatch(const FPointMassModel& Model, double Time,
									   std::span<const FProjectileState> States, double TimeStep,
									   std::span<FStepResult> OutResults) const override;
	};

	class OGIVACORE_API FRk4Integrator final : public IIntegrator
	{
	public:
		[[nodiscard]] EError StepBatch(const FPointMassModel& Model, double Time,
									   std::span<const FProjectileState> States, double TimeStep,
									   std::span<FStepResult> OutResults) const override;
	};

	class OGIVACORE_API FDormandPrinceIntegrator final : public IIntegrator
	{
	public:
		[[nodiscard]] EError StepBatch(const FPointMassModel& Model, double Time,
									   std::span<const FProjectileState> States, double TimeStep,
									   std::span<FStepResult> OutResults) const override;
	};
}
