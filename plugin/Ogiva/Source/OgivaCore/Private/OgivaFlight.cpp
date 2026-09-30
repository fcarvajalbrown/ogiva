#include "OgivaFlight.h"

#include <algorithm>
#include <cmath>
#include <span>

namespace Ogiva
{
	namespace
	{
		constexpr double StepSafety = 0.9;
		constexpr double MinStepFactor = 0.2;
		constexpr double MaxStepFactor = 5.0;
		constexpr int MaxRootIterations = 100;

		struct FNode
		{
			double Time = 0.0;
			FProjectileState State;
			FStateDerivative Derivative;
		};

		struct FHermiteWeights
		{
			double Start = 0.0;
			double StartRate = 0.0;
			double StartCurvature = 0.0;
			double EndCurvature = 0.0;
			double EndRate = 0.0;
			double End = 0.0;
		};

		FHermiteWeights HermiteWeights(double T)
		{
			const double T2 = T * T;
			const double T3 = T2 * T;
			const double T4 = T3 * T;
			const double T5 = T4 * T;
			return {
				.Start = 1.0 - 10.0 * T3 + 15.0 * T4 - 6.0 * T5,
				.StartRate = T - 6.0 * T3 + 8.0 * T4 - 3.0 * T5,
				.StartCurvature = 0.5 * T2 - 1.5 * T3 + 1.5 * T4 - 0.5 * T5,
				.EndCurvature = 0.5 * T3 - T4 + 0.5 * T5,
				.EndRate = -4.0 * T3 + 7.0 * T4 - 3.0 * T5,
				.End = 10.0 * T3 - 15.0 * T4 + 6.0 * T5,
			};
		}

		FHermiteWeights HermiteSlopeWeights(double T)
		{
			const double T2 = T * T;
			const double T3 = T2 * T;
			const double T4 = T3 * T;
			return {
				.Start = -30.0 * T2 + 60.0 * T3 - 30.0 * T4,
				.StartRate = 1.0 - 18.0 * T2 + 32.0 * T3 - 15.0 * T4,
				.StartCurvature = T - 4.5 * T2 + 6.0 * T3 - 2.5 * T4,
				.EndCurvature = 1.5 * T2 - 4.0 * T3 + 2.5 * T4,
				.EndRate = -12.0 * T2 + 28.0 * T3 - 15.0 * T4,
				.End = 30.0 * T2 - 60.0 * T3 + 30.0 * T4,
			};
		}

		FVector3 Interpolate(const FNode& Start, const FNode& End, const FHermiteWeights& Weights)
		{
			const double H = End.Time - Start.Time;
			const double H2 = H * H;
			return Start.State.Position * Weights.Start + Start.Derivative.Velocity * (H * Weights.StartRate) +
				   Start.Derivative.Acceleration * (H2 * Weights.StartCurvature) +
				   End.Derivative.Acceleration * (H2 * Weights.EndCurvature) +
				   End.Derivative.Velocity * (H * Weights.EndRate) + End.State.Position * Weights.End;
		}

		bool IsFinite(const FVector3& Vector)
		{
			return std::isfinite(Vector.X) && std::isfinite(Vector.Y) && std::isfinite(Vector.Z);
		}

		bool IsFinite(const FProjectileState& State)
		{
			return IsFinite(State.Position) && IsFinite(State.Velocity);
		}

		bool IsPositive(double Value)
		{
			return std::isfinite(Value) && Value > 0.0;
		}

		double SignedDistance(const FPlane& Plane, const FVector3& Position)
		{
			return Dot(Plane.Normal, Position) - Plane.Offset;
		}

		double CrossingFraction(const FNode& Start, const FNode& End, const FPlane& Plane)
		{
			const double StartDistance = SignedDistance(Plane, Start.State.Position);
			const double EndDistance = SignedDistance(Plane, End.State.Position);
			const auto Combine = [&](const FHermiteWeights& Weights)
			{
				return Dot(Plane.Normal, Interpolate(Start, End, Weights)) -
					   Plane.Offset * (Weights.Start + Weights.End);
			};

			const bool StartsNegative = StartDistance < 0.0;
			double Lower = 0.0;
			double Upper = 1.0;
			double T = StartDistance / (StartDistance - EndDistance);
			for (int Iteration = 0; Iteration < MaxRootIterations; ++Iteration)
			{
				const double Value = Combine(HermiteWeights(T));
				if (Value == 0.0)
				{
					return T;
				}
				if ((Value < 0.0) == StartsNegative)
				{
					Lower = T;
				}
				else
				{
					Upper = T;
				}
				const double Newton = T - Value / Combine(HermiteSlopeWeights(T));
				const double Next = Newton > Lower && Newton < Upper ? Newton : 0.5 * (Lower + Upper);
				if (Next == T)
				{
					return T;
				}
				T = Next;
			}
			return T;
		}

		FPlaneCrossing InterpolateCrossing(const FNode& Start, const FNode& End, double T)
		{
			const double H = End.Time - Start.Time;
			return {
				.Time = std::lerp(Start.Time, End.Time, T),
				.State =
					{
						.Position = Interpolate(Start, End, HermiteWeights(T)),
						.Velocity = Interpolate(Start, End, HermiteSlopeWeights(T)) / H,
					},
			};
		}

		double ScaledSquares(const FVector3& Error, const FVector3& Start, const FVector3& End,
							 const FFlightSettings& Settings)
		{
			const auto Term = [&](double ComponentError, double StartValue, double EndValue)
			{
				const double Scale = Settings.AbsoluteTolerance +
									 Settings.RelativeTolerance * std::max(std::abs(StartValue), std::abs(EndValue));
				const double Ratio = ComponentError / Scale;
				return Ratio * Ratio;
			};
			return Term(Error.X, Start.X, End.X) + Term(Error.Y, Start.Y, End.Y) + Term(Error.Z, Start.Z, End.Z);
		}

		double ErrorNorm(const FStepResult& Result, const FProjectileState& Start, const FFlightSettings& Settings)
		{
			constexpr double ComponentCount = 6.0;
			const double Sum =
				ScaledSquares(Result.LocalError.Position, Start.Position, Result.State.Position, Settings) +
				ScaledSquares(Result.LocalError.Velocity, Start.Velocity, Result.State.Velocity, Settings);
			return std::sqrt(Sum / ComponentCount);
		}

		bool IsValidStepControl(const IIntegrator& Integrator, const FFlightSettings& Settings)
		{
			switch (Settings.StepControl)
			{
				case EStepControl::Fixed:
					return true;
				case EStepControl::Adaptive:
					return Integrator.ErrorEstimateOrder() > 0 && IsPositive(Settings.AbsoluteTolerance) &&
						   std::isfinite(Settings.RelativeTolerance) && Settings.RelativeTolerance >= 0.0;
			}
			return false;
		}

		bool IsValidInput(const IIntegrator& Integrator, const FProjectileState& Initial, const FPlane& Plane,
						  const FFlightSettings& Settings)
		{
			return IsFinite(Initial) && IsFinite(Plane.Normal) && SquaredLength(Plane.Normal) > 0.0 &&
				   std::isfinite(Plane.Offset) && IsPositive(Settings.TimeStep) && IsPositive(Settings.MaxTime) &&
				   IsValidStepControl(Integrator, Settings);
		}
	}

	EError FlyToPlane(const IIntegrator& Integrator, const FPointMassModel& Model, const FProjectileState& Initial,
					  const FPlane& Plane, const FFlightSettings& Settings, FPlaneCrossing& OutCrossing)
	{
		if (!IsValidInput(Integrator, Initial, Plane, Settings))
		{
			return EError::InvalidArgument;
		}

		const double InitialDistance = SignedDistance(Plane, Initial.Position);
		if (InitialDistance == 0.0)
		{
			OutCrossing = {.Time = 0.0, .State = Initial};
			return EError::None;
		}

		const bool StartsNegative = InitialDistance < 0.0;
		const bool UsesAdaptiveSteps = Settings.StepControl == EStepControl::Adaptive;
		const double ErrorExponent = UsesAdaptiveSteps ? -1.0 / Integrator.ErrorEstimateOrder() : 0.0;
		FNode Current{.Time = 0.0, .State = Initial, .Derivative = Model.Derivative(0.0, Initial)};
		double TimeStep = Settings.TimeStep;

		while (Current.Time < Settings.MaxTime)
		{
			const double Remaining = Settings.MaxTime - Current.Time;
			const bool ReachesMaxTime = TimeStep >= Remaining;
			const double StepSize = ReachesMaxTime ? Remaining : TimeStep;

			FStepResult Result;
			const EError StepError = Integrator.StepBatch(Model, Current.Time, std::span(&Current.State, 1), StepSize,
														  std::span(&Result, 1));
			if (StepError != EError::None)
			{
				return StepError;
			}
			if (!IsFinite(Result.State))
			{
				return EError::NotConverged;
			}

			if (UsesAdaptiveSteps)
			{
				const double Norm = ErrorNorm(Result, Current.State, Settings);
				const double Factor =
					std::clamp(StepSafety * std::pow(Norm, ErrorExponent), MinStepFactor, MaxStepFactor);
				TimeStep = StepSize * Factor;
				if (Norm > 1.0)
				{
					if (Current.Time + TimeStep == Current.Time)
					{
						return EError::NotConverged;
					}
					continue;
				}
			}

			const FNode Next{
				.Time = ReachesMaxTime ? Settings.MaxTime : Current.Time + StepSize,
				.State = Result.State,
				.Derivative = Result.EndDerivative,
			};
			const double NextDistance = SignedDistance(Plane, Next.State.Position);
			if (NextDistance == 0.0 || (NextDistance < 0.0) != StartsNegative)
			{
				OutCrossing = InterpolateCrossing(Current, Next, CrossingFraction(Current, Next, Plane));
				return EError::None;
			}
			Current = Next;
		}
		return EError::TargetNotReached;
	}
}
