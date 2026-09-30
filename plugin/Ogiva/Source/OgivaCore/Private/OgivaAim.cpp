#include "OgivaAim.h"

#include "OgivaAimFallback.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace Ogiva
{
	namespace
	{
		constexpr int MaxBroydenIterations = 25;
		constexpr int MaxAlternations = 40;
		constexpr int MaxBracketExpansions = 60;
		constexpr int MaxRootIterations = 100;
		constexpr int MaxPeakIterations = 100;
		constexpr double MaxAngleStep = 0.2;
		constexpr double BracketGrowth = 1.6;
		constexpr double MinElevationOffset = 1e-6;
		constexpr double RoundoffMissScale = 64.0 * std::numeric_limits<double>::epsilon();
		constexpr double ElevationLimit = 0.5 * std::numbers::pi - 1e-3;
		constexpr double GoldenRatio = std::numbers::phi - 1.0;

		struct FTrial
		{
			double Elevation = 0.0;
			double Azimuth = 0.0;
			double Vertical = 0.0;
			double Lateral = 0.0;
			FPlaneCrossing Crossing;
		};

		struct FJacobian
		{
			double VerticalByElevation = 0.0;
			double VerticalByAzimuth = 0.0;
			double LateralByElevation = 0.0;
			double LateralByAzimuth = 0.0;
		};

		class FAimProblem
		{
		public:
			FAimProblem(const IIntegrator& InIntegrator, const FPointMassModel& InModel, const FVector3& InMuzzle,
						double InSpeed, const FVector3& InTarget, const FFlightSettings& InSettings)
				: Integrator(&InIntegrator)
				, Model(&InModel)
				, Muzzle(InMuzzle)
				, Speed(InSpeed)
				, Target(InTarget)
				, Settings(InSettings)
				, Range(Length(InTarget - InMuzzle))
				, Sight((InTarget - InMuzzle) / Range)
				, Up(Normalized(FVector3::Up() - Sight * Sight.Z))
				, Left(Cross(Up, Sight))
				, MissTolerance(std::max(
					  {InSettings.AbsoluteTolerance, InSettings.RelativeTolerance * Range, RoundoffMissScale * Range}))
			{
			}

			[[nodiscard]] double SightElevation() const { return std::atan2(Sight.Z, std::hypot(Sight.X, Sight.Y)); }
			[[nodiscard]] double SightAzimuth() const { return std::atan2(Sight.Y, Sight.X); }
			[[nodiscard]] double GetRange() const { return Range; }
			[[nodiscard]] double GetSpeed() const { return Speed; }

			[[nodiscard]] bool IsConverged(const FTrial& Trial) const
			{
				return std::hypot(Trial.Vertical, Trial.Lateral) <= MissTolerance;
			}

			[[nodiscard]] bool IsVerticallyConverged(const FTrial& Trial) const
			{
				return std::abs(Trial.Vertical) <= MissTolerance;
			}

			[[nodiscard]] EError Evaluate(double Elevation, double Azimuth, FTrial& OutTrial) const
			{
				const FProjectileState Launch{
					.Position = Muzzle,
					.Velocity = LaunchDirection(Elevation, Azimuth) * Speed,
				};
				const FPlane Plane{.Normal = Sight, .Offset = Dot(Sight, Target)};
				FPlaneCrossing Crossing;
				const EError Error = FlyToPlane(*Integrator, *Model, Launch, Plane, Settings, Crossing);
				if (Error != EError::None)
				{
					return Error;
				}
				const FVector3 Miss = Crossing.State.Position - Target;
				OutTrial = {
					.Elevation = Elevation,
					.Azimuth = Azimuth,
					.Vertical = Dot(Miss, Up),
					.Lateral = Dot(Miss, Left),
					.Crossing = Crossing,
				};
				return EError::None;
			}

		private:
			static FVector3 Normalized(const FVector3& Vector) { return Vector / Length(Vector); }

			const IIntegrator* Integrator;
			const FPointMassModel* Model;
			FVector3 Muzzle;
			double Speed;
			FVector3 Target;
			FFlightSettings Settings;
			double Range;
			FVector3 Sight;
			FVector3 Up;
			FVector3 Left;
			double MissTolerance;
		};

		bool IsFinite(const FVector3& Vector)
		{
			return std::isfinite(Vector.X) && std::isfinite(Vector.Y) && std::isfinite(Vector.Z);
		}

		double VacuumDropAngle(const FAimProblem& Problem, double SightElevation)
		{
			const double Speed = Problem.GetSpeed();
			return 0.5 * StandardGravity * std::cos(SightElevation) * Problem.GetRange() / (Speed * Speed);
		}

		bool SolveBroyden(const FAimProblem& Problem, FTrial& InOutTrial)
		{
			const double Range = Problem.GetRange();
			FJacobian Jacobian{
				.VerticalByElevation = Range,
				.LateralByAzimuth = Range * std::cos(InOutTrial.Elevation),
			};
			for (int Iteration = 0; Iteration < MaxBroydenIterations; ++Iteration)
			{
				if (Problem.IsConverged(InOutTrial))
				{
					return Jacobian.VerticalByElevation > 0.0;
				}
				const double Determinant = Jacobian.VerticalByElevation * Jacobian.LateralByAzimuth -
										   Jacobian.VerticalByAzimuth * Jacobian.LateralByElevation;
				double ElevationStep = (Jacobian.VerticalByAzimuth * InOutTrial.Lateral -
										Jacobian.LateralByAzimuth * InOutTrial.Vertical) /
									   Determinant;
				double AzimuthStep = (Jacobian.LateralByElevation * InOutTrial.Vertical -
									  Jacobian.VerticalByElevation * InOutTrial.Lateral) /
									 Determinant;
				if (!std::isfinite(ElevationStep) || !std::isfinite(AzimuthStep))
				{
					return false;
				}
				const double StepSize = std::hypot(ElevationStep, AzimuthStep);
				if (StepSize > MaxAngleStep)
				{
					ElevationStep *= MaxAngleStep / StepSize;
					AzimuthStep *= MaxAngleStep / StepSize;
				}

				FTrial Next;
				if (Problem.Evaluate(InOutTrial.Elevation + ElevationStep, InOutTrial.Azimuth + AzimuthStep, Next) !=
					EError::None)
				{
					return false;
				}
				const double StepSquared = ElevationStep * ElevationStep + AzimuthStep * AzimuthStep;
				const double VerticalSurprise = Next.Vertical - InOutTrial.Vertical -
												Jacobian.VerticalByElevation * ElevationStep -
												Jacobian.VerticalByAzimuth * AzimuthStep;
				const double LateralSurprise = Next.Lateral - InOutTrial.Lateral -
											   Jacobian.LateralByElevation * ElevationStep -
											   Jacobian.LateralByAzimuth * AzimuthStep;
				Jacobian.VerticalByElevation += VerticalSurprise * ElevationStep / StepSquared;
				Jacobian.VerticalByAzimuth += VerticalSurprise * AzimuthStep / StepSquared;
				Jacobian.LateralByElevation += LateralSurprise * ElevationStep / StepSquared;
				Jacobian.LateralByAzimuth += LateralSurprise * AzimuthStep / StepSquared;
				InOutTrial = Next;
			}
			return Problem.IsConverged(InOutTrial) && Jacobian.VerticalByElevation > 0.0;
		}

		bool IsStrictlyBetween(double Value, double Low, double High)
		{
			return Value > Low && Value < High;
		}

		EError FindVerticalRoot(const FAimProblem& Problem, FTrial Low, FTrial High, FTrial& OutTrial)
		{
			if (Problem.IsVerticallyConverged(High))
			{
				OutTrial = High;
				return EError::None;
			}
			double LowValue = Low.Vertical;
			double HighValue = High.Vertical;
			int LastMovedSide = 0;
			for (int Iteration = 0; Iteration < MaxRootIterations; ++Iteration)
			{
				double Elevation = (Low.Elevation * HighValue - High.Elevation * LowValue) / (HighValue - LowValue);
				if (!IsStrictlyBetween(Elevation, Low.Elevation, High.Elevation))
				{
					Elevation = 0.5 * (Low.Elevation + High.Elevation);
				}
				if (!IsStrictlyBetween(Elevation, Low.Elevation, High.Elevation))
				{
					OutTrial = std::abs(Low.Vertical) < std::abs(High.Vertical) ? Low : High;
					return EError::None;
				}
				FTrial Middle;
				const EError Error = Problem.Evaluate(Elevation, Low.Azimuth, Middle);
				if (Error != EError::None)
				{
					return Error;
				}
				if (Problem.IsVerticallyConverged(Middle))
				{
					OutTrial = Middle;
					return EError::None;
				}
				if (Middle.Vertical < 0.0)
				{
					Low = Middle;
					LowValue = Middle.Vertical;
					HighValue *= LastMovedSide < 0 ? 0.5 : 1.0;
					LastMovedSide = -1;
				}
				else
				{
					High = Middle;
					HighValue = Middle.Vertical;
					LowValue *= LastMovedSide > 0 ? 0.5 : 1.0;
					LastMovedSide = 1;
				}
			}
			return EError::NotConverged;
		}

		EError EvaluateForPeak(const FAimProblem& Problem, double Elevation, double Azimuth, FTrial& OutTrial)
		{
			const EError Error = Problem.Evaluate(Elevation, Azimuth, OutTrial);
			if (Error == EError::TargetNotReached)
			{
				FTrial Unreachable;
				Unreachable.Elevation = Elevation;
				Unreachable.Azimuth = Azimuth;
				Unreachable.Vertical = -std::numeric_limits<double>::infinity();
				OutTrial = Unreachable;
				return EError::None;
			}
			return Error;
		}

		EError FindPeak(const FAimProblem& Problem, double Low, double High, double Azimuth, FTrial& OutPeak)
		{
			FTrial Inner;
			FTrial Outer;
			double InnerElevation = High - GoldenRatio * (High - Low);
			double OuterElevation = Low + GoldenRatio * (High - Low);
			EError Error = EvaluateForPeak(Problem, InnerElevation, Azimuth, Inner);
			if (Error == EError::None)
			{
				Error = EvaluateForPeak(Problem, OuterElevation, Azimuth, Outer);
			}
			if (Error != EError::None)
			{
				return Error;
			}
			for (int Iteration = 0; Iteration < MaxPeakIterations && Inner.Vertical <= 0.0 && Outer.Vertical <= 0.0;
				 ++Iteration)
			{
				if (Inner.Vertical >= Outer.Vertical)
				{
					High = OuterElevation;
					Outer = Inner;
					OuterElevation = InnerElevation;
					InnerElevation = High - GoldenRatio * (High - Low);
					Error = EvaluateForPeak(Problem, InnerElevation, Azimuth, Inner);
				}
				else
				{
					Low = InnerElevation;
					Inner = Outer;
					InnerElevation = OuterElevation;
					OuterElevation = Low + GoldenRatio * (High - Low);
					Error = EvaluateForPeak(Problem, OuterElevation, Azimuth, Outer);
				}
				if (Error != EError::None)
				{
					return Error;
				}
				if (InnerElevation >= OuterElevation)
				{
					break;
				}
			}
			OutPeak = Inner.Vertical >= Outer.Vertical ? Inner : Outer;
			return EError::None;
		}

		EError SolveElevation(const FAimProblem& Problem, double Azimuth, FTrial& OutTrial)
		{
			const double SightElevation = Problem.SightElevation();
			FTrial Low;
			EError Error = Problem.Evaluate(SightElevation, Azimuth, Low);
			if (Error != EError::None)
			{
				return Error;
			}
			double Step = std::max(VacuumDropAngle(Problem, SightElevation), MinElevationOffset);
			for (int Expansion = 0; Expansion < MaxBracketExpansions && Low.Vertical > 0.0; ++Expansion)
			{
				Error = Problem.Evaluate(std::max(Low.Elevation - Step, -ElevationLimit), Azimuth, Low);
				if (Error != EError::None)
				{
					return Error;
				}
				Step *= BracketGrowth;
			}
			if (Low.Vertical > 0.0)
			{
				return EError::NotConverged;
			}

			FTrial Previous = Low;
			for (int Expansion = 0; Expansion < MaxBracketExpansions; ++Expansion)
			{
				const double Elevation = std::min(Low.Elevation + Step, ElevationLimit);
				FTrial High;
				const EError HighError = Problem.Evaluate(Elevation, Azimuth, High);
				if (HighError == EError::None && High.Vertical >= 0.0)
				{
					return FindVerticalRoot(Problem, Low, High, OutTrial);
				}
				if (HighError != EError::None && HighError != EError::TargetNotReached)
				{
					return HighError;
				}
				if (HighError == EError::TargetNotReached || High.Vertical < Low.Vertical ||
					Elevation >= ElevationLimit)
				{
					FTrial Peak;
					Error = FindPeak(Problem, Previous.Elevation, Elevation, Azimuth, Peak);
					if (Error != EError::None)
					{
						return Error;
					}
					if (Peak.Vertical < 0.0)
					{
						return EError::OutOfRange;
					}
					return FindVerticalRoot(Problem, Peak.Elevation < Low.Elevation ? Previous : Low, Peak, OutTrial);
				}
				Previous = Low;
				Low = High;
				Step *= BracketGrowth;
			}
			return EError::OutOfRange;
		}

		EError SolveAlternating(const FAimProblem& Problem, double Azimuth, FTrial& OutTrial)
		{
			for (int Alternation = 0; Alternation < MaxAlternations; ++Alternation)
			{
				const EError Error = SolveElevation(Problem, Azimuth, OutTrial);
				if (Error != EError::None)
				{
					return Error;
				}
				if (Problem.IsConverged(OutTrial))
				{
					return EError::None;
				}
				Azimuth -= OutTrial.Lateral / (Problem.GetRange() * std::cos(OutTrial.Elevation));
			}
			return EError::NotConverged;
		}

		bool IsValidAimInput(const FVector3& Muzzle, double MuzzleSpeed, const FVector3& Target)
		{
			const FVector3 Sight = Target - Muzzle;
			return IsFinite(Muzzle) && IsFinite(Target) && std::isfinite(MuzzleSpeed) && MuzzleSpeed > 0.0 &&
				   (Sight.X != 0.0 || Sight.Y != 0.0);
		}

		FAimSolution ToSolution(const FTrial& Trial)
		{
			return {.Elevation = Trial.Elevation, .Azimuth = Trial.Azimuth, .Crossing = Trial.Crossing};
		}

		FVector3 SightPoint(const FZeroRequest& Request, double Range)
		{
			return Request.Muzzle + FVector3{.X = Range, .Y = 0.0, .Z = Request.SightHeight};
		}
	}

	FVector3 LaunchDirection(double Elevation, double Azimuth)
	{
		const double Horizontal = std::cos(Elevation);
		return {
			.X = Horizontal * std::cos(Azimuth),
			.Y = Horizontal * std::sin(Azimuth),
			.Z = std::sin(Elevation),
		};
	}

	EError SolveAim(const IIntegrator& Integrator, const FPointMassModel& Model, const FVector3& Muzzle,
					double MuzzleSpeed, const FVector3& Target, const FFlightSettings& Settings, FAimSolution& OutAim)
	{
		if (!IsValidAimInput(Muzzle, MuzzleSpeed, Target))
		{
			return EError::InvalidArgument;
		}
		const FAimProblem Problem(Integrator, Model, Muzzle, MuzzleSpeed, Target, Settings);

		const double SightElevation = Problem.SightElevation();
		FTrial Trial;
		const EError Error =
			Problem.Evaluate(SightElevation + VacuumDropAngle(Problem, SightElevation), Problem.SightAzimuth(), Trial);
		if (Error == EError::InvalidArgument)
		{
			return Error;
		}
		if (Error != EError::None || !SolveBroyden(Problem, Trial))
		{
			return AimFallback::SolveByBracketing(Integrator, Model, Muzzle, MuzzleSpeed, Target, Settings, OutAim);
		}
		OutAim = ToSolution(Trial);
		return EError::None;
	}

	EError AimFallback::SolveByBracketing(const IIntegrator& Integrator, const FPointMassModel& Model,
										  const FVector3& Muzzle, double MuzzleSpeed, const FVector3& Target,
										  const FFlightSettings& Settings, FAimSolution& OutAim)
	{
		if (!IsValidAimInput(Muzzle, MuzzleSpeed, Target))
		{
			return EError::InvalidArgument;
		}
		const FAimProblem Problem(Integrator, Model, Muzzle, MuzzleSpeed, Target, Settings);
		FTrial Trial;
		const EError Error = SolveAlternating(Problem, Problem.SightAzimuth(), Trial);
		if (Error != EError::None)
		{
			return Error;
		}
		OutAim = ToSolution(Trial);
		return EError::None;
	}

	EError SolveZero(const IIntegrator& Integrator, const FPointMassModel& Model, const FZeroRequest& Request,
					 const FFlightSettings& Settings, FAimSolution& OutZero)
	{
		if (!std::isfinite(Request.SightHeight) || !std::isfinite(Request.ZeroRange) || Request.ZeroRange <= 0.0)
		{
			return EError::InvalidArgument;
		}
		return SolveAim(Integrator, Model, Request.Muzzle, Request.MuzzleSpeed, SightPoint(Request, Request.ZeroRange),
						Settings, OutZero);
	}

	EError SolveHold(const IIntegrator& Integrator, const FPointMassModel& Model, const FZeroRequest& Request,
					 const FAimSolution& Zero, double Range, const FFlightSettings& Settings, FHold& OutHold)
	{
		if (!std::isfinite(Request.SightHeight) || !std::isfinite(Range) || Range <= 0.0 ||
			!std::isfinite(Zero.Elevation) || !std::isfinite(Zero.Azimuth) ||
			!IsValidAimInput(Request.Muzzle, Request.MuzzleSpeed, SightPoint(Request, Range)))
		{
			return EError::InvalidArgument;
		}
		const FVector3 Target = SightPoint(Request, Range);
		const FProjectileState Launch{
			.Position = Request.Muzzle,
			.Velocity = LaunchDirection(Zero.Elevation, Zero.Azimuth) * Request.MuzzleSpeed,
		};
		const FPlane Plane{.Normal = {.X = 1.0, .Y = 0.0, .Z = 0.0}, .Offset = Target.X};
		FPlaneCrossing Crossing;
		EError Error = FlyToPlane(Integrator, Model, Launch, Plane, Settings, Crossing);
		if (Error != EError::None)
		{
			return Error;
		}
		FAimSolution Aim;
		Error = SolveAim(Integrator, Model, Request.Muzzle, Request.MuzzleSpeed, Target, Settings, Aim);
		if (Error != EError::None)
		{
			return Error;
		}
		OutHold = {
			.Drop = Target.Z - Crossing.State.Position.Z,
			.Drift = Crossing.State.Position.Y - Target.Y,
			.Elevation = Aim.Elevation - Zero.Elevation,
			.Windage = Aim.Azimuth - Zero.Azimuth,
			.Crossing = Crossing,
		};
		return EError::None;
	}
}
