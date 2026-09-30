#include "OgivaIntegrator.h"

namespace Ogiva
{
	namespace
	{
		FStateDerivative operator*(const FStateDerivative& Derivative, double Scale)
		{
			return {.Velocity = Derivative.Velocity * Scale, .Acceleration = Derivative.Acceleration * Scale};
		}

		FStateDerivative operator+(const FStateDerivative& Left, const FStateDerivative& Right)
		{
			return {.Velocity = Left.Velocity + Right.Velocity, .Acceleration = Left.Acceleration + Right.Acceleration};
		}

		constexpr FProjectileState ZeroState{.Position = FVector3::Zero(), .Velocity = FVector3::Zero()};

		template <typename TStep>
		EError StepEach(std::span<const FProjectileState> States, std::span<FStepResult> OutResults, TStep Step)
		{
			if (States.size() != OutResults.size())
			{
				return EError::InvalidArgument;
			}
			auto Out = OutResults.begin();
			for (const FProjectileState& State : States)
			{
				*Out = Step(State);
				++Out;
			}
			return EError::None;
		}

		FStepResult EulerStep(const FPointMassModel& Model, double Time, const FProjectileState& State, double H)
		{
			const FProjectileState Next = Advance(State, Model.Derivative(Time, State), H);
			return {.State = Next, .EndDerivative = Model.Derivative(Time + H, Next), .LocalError = ZeroState};
		}

		FStepResult Rk4Step(const FPointMassModel& Model, double Time, const FProjectileState& State, double H)
		{
			const double HalfH = 0.5 * H;
			const FStateDerivative K1 = Model.Derivative(Time, State);
			const FStateDerivative K2 = Model.Derivative(Time + HalfH, Advance(State, K1, HalfH));
			const FStateDerivative K3 = Model.Derivative(Time + HalfH, Advance(State, K2, HalfH));
			const FStateDerivative K4 = Model.Derivative(Time + H, Advance(State, K3, H));
			const FProjectileState Next = Advance(State, K1 + K2 * 2.0 + K3 * 2.0 + K4, H / 6.0);
			return {.State = Next, .EndDerivative = Model.Derivative(Time + H, Next), .LocalError = ZeroState};
		}

		namespace DormandPrince
		{
			constexpr double C2 = 1.0 / 5.0;
			constexpr double C3 = 3.0 / 10.0;
			constexpr double C4 = 4.0 / 5.0;
			constexpr double C5 = 8.0 / 9.0;

			constexpr double A21 = 1.0 / 5.0;
			constexpr double A31 = 3.0 / 40.0;
			constexpr double A32 = 9.0 / 40.0;
			constexpr double A41 = 44.0 / 45.0;
			constexpr double A42 = -56.0 / 15.0;
			constexpr double A43 = 32.0 / 9.0;
			constexpr double A51 = 19372.0 / 6561.0;
			constexpr double A52 = -25360.0 / 2187.0;
			constexpr double A53 = 64448.0 / 6561.0;
			constexpr double A54 = -212.0 / 729.0;
			constexpr double A61 = 9017.0 / 3168.0;
			constexpr double A62 = -355.0 / 33.0;
			constexpr double A63 = 46732.0 / 5247.0;
			constexpr double A64 = 49.0 / 176.0;
			constexpr double A65 = -5103.0 / 18656.0;

			constexpr double B1 = 35.0 / 384.0;
			constexpr double B3 = 500.0 / 1113.0;
			constexpr double B4 = 125.0 / 192.0;
			constexpr double B5 = -2187.0 / 6784.0;
			constexpr double B6 = 11.0 / 84.0;

			constexpr double E1 = 71.0 / 57600.0;
			constexpr double E3 = -71.0 / 16695.0;
			constexpr double E4 = 71.0 / 1920.0;
			constexpr double E5 = -17253.0 / 339200.0;
			constexpr double E6 = 22.0 / 525.0;
			constexpr double E7 = -1.0 / 40.0;
		}

		FStepResult DormandPrinceStep(const FPointMassModel& Model, double Time, const FProjectileState& State,
									  double H)
		{
			using namespace DormandPrince;
			const FStateDerivative K1 = Model.Derivative(Time, State);
			const FStateDerivative K2 = Model.Derivative(Time + C2 * H, Advance(State, K1 * A21, H));
			const FStateDerivative K3 = Model.Derivative(Time + C3 * H, Advance(State, K1 * A31 + K2 * A32, H));
			const FStateDerivative K4 =
				Model.Derivative(Time + C4 * H, Advance(State, K1 * A41 + K2 * A42 + K3 * A43, H));
			const FStateDerivative K5 =
				Model.Derivative(Time + C5 * H, Advance(State, K1 * A51 + K2 * A52 + K3 * A53 + K4 * A54, H));
			const FStateDerivative K6 =
				Model.Derivative(Time + H, Advance(State, K1 * A61 + K2 * A62 + K3 * A63 + K4 * A64 + K5 * A65, H));
			const FProjectileState Next = Advance(State, K1 * B1 + K3 * B3 + K4 * B4 + K5 * B5 + K6 * B6, H);
			const FStateDerivative K7 = Model.Derivative(Time + H, Next);
			return {
				.State = Next,
				.EndDerivative = K7,
				.LocalError = Advance(ZeroState, K1 * E1 + K3 * E3 + K4 * E4 + K5 * E5 + K6 * E6 + K7 * E7, H),
			};
		}
	}

	EError FEulerIntegrator::StepBatch(const FPointMassModel& Model, double Time,
									   std::span<const FProjectileState> States, double TimeStep,
									   std::span<FStepResult> OutResults) const
	{
		return StepEach(States, OutResults,
						[&](const FProjectileState& State) { return EulerStep(Model, Time, State, TimeStep); });
	}

	EError FRk4Integrator::StepBatch(const FPointMassModel& Model, double Time,
									 std::span<const FProjectileState> States, double TimeStep,
									 std::span<FStepResult> OutResults) const
	{
		return StepEach(States, OutResults,
						[&](const FProjectileState& State) { return Rk4Step(Model, Time, State, TimeStep); });
	}

	EError FDormandPrinceIntegrator::StepBatch(const FPointMassModel& Model, double Time,
											   std::span<const FProjectileState> States, double TimeStep,
											   std::span<FStepResult> OutResults) const
	{
		return StepEach(States, OutResults,
						[&](const FProjectileState& State) { return DormandPrinceStep(Model, Time, State, TimeStep); });
	}

	int FEulerIntegrator::ErrorEstimateOrder() const
	{
		return 0;
	}

	int FRk4Integrator::ErrorEstimateOrder() const
	{
		return 0;
	}

	int FDormandPrinceIntegrator::ErrorEstimateOrder() const
	{
		return 5;
	}
}
