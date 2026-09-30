#include "OgivaIntegrator.h"

#include <catch_amalgamated.hpp>

#include <array>
#include <cmath>
#include <numbers>

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;
using Ogiva::EError;
using Ogiva::FDormandPrinceIntegrator;
using Ogiva::FEulerIntegrator;
using Ogiva::FPchipCurve;
using Ogiva::FPointMassModel;
using Ogiva::FPointMassParams;
using Ogiva::FProjectileState;
using Ogiva::FRk4Integrator;
using Ogiva::FStepResult;
using Ogiva::FVector3;
using Ogiva::IIntegrator;

namespace
{
	constexpr FVector3 Gravity{.X = 0.0, .Y = 0.0, .Z = -Ogiva::StandardGravity};
	constexpr FProjectileState Muzzle{
		.Position = {.X = 0.0, .Y = 0.0, .Z = 1.5},
		.Velocity = {.X = 850.0, .Y = 1.0, .Z = 8.0},
	};
	constexpr double AirDensity = 1.225;
	constexpr double SpeedOfSound = 340.294;
	double BallisticCoefficient()
	{
		return Ogiva::BallisticCoefficientFromImperial(0.25);
	}

	FPchipCurve MakeCurve(double StartCd, double EndCd)
	{
		FPchipCurve Curve;
		REQUIRE(FPchipCurve::Create(std::array{0.0, 5.0}, std::array{StartCd, EndCd}, Curve) == EError::None);
		return Curve;
	}

	FPointMassParams MakeParams(const FPchipCurve& Curve)
	{
		return {
			.DragCurve = &Curve,
			.BallisticCoefficient = BallisticCoefficient(),
			.AirDensity = AirDensity,
			.SpeedOfSound = SpeedOfSound,
			.Wind = {.X = -2.0, .Y = 3.0, .Z = 0.0},
			.Gravity = Gravity,
			.EarthRotation = Ogiva::EarthRotationInFrame(-0.58, 0.4),
		};
	}

	FProjectileState Fly(const IIntegrator& Integrator, const FPointMassModel& Model, double Duration, int Steps)
	{
		const double TimeStep = Duration / Steps;
		FProjectileState State = Muzzle;
		std::array<FStepResult, 1> Result{};
		for (int Index = 0; Index < Steps; ++Index)
		{
			REQUIRE(Integrator.StepBatch(Model, Index * TimeStep, std::span(&State, 1), TimeStep, Result) ==
					EError::None);
			State = Result.front().State;
		}
		return State;
	}

	double ObservedOrder(const IIntegrator& Integrator, const FPointMassModel& Model, double Duration, int CoarseSteps,
						 const FProjectileState& Expected)
	{
		const double CoarseError =
			Ogiva::Length(Fly(Integrator, Model, Duration, CoarseSteps).Position - Expected.Position);
		const double FineError =
			Ogiva::Length(Fly(Integrator, Model, Duration, 2 * CoarseSteps).Position - Expected.Position);
		return std::log2(CoarseError / FineError);
	}
}

TEST_CASE("A multi-step vacuum flight stays on the closed-form parabola", "[trajectory]")
{
	const FPchipCurve Curve = MakeCurve(0.3, 0.3);
	FPointMassParams Params = MakeParams(Curve);
	Params.AirDensity = 0.0;
	Params.EarthRotation = FVector3::Zero();
	const FPointMassModel Model(Params);
	constexpr double Duration = 2.0;

	const FVector3 Position = Muzzle.Position + Muzzle.Velocity * Duration + Gravity * (0.5 * Duration * Duration);
	const FRk4Integrator Rk4;
	const FDormandPrinceIntegrator DormandPrince;
	for (const IIntegrator* Integrator : std::array<const IIntegrator*, 2>{&Rk4, &DormandPrince})
	{
		const FProjectileState Final = Fly(*Integrator, Model, Duration, 200);
		CHECK_THAT(Final.Position.X, WithinRel(Position.X, 1e-14));
		CHECK_THAT(Final.Position.Y, WithinRel(Position.Y, 1e-14));
		CHECK_THAT(Final.Position.Z, WithinAbs(Position.Z, 1e-11));
	}
}

TEST_CASE("Dormand-Prince matches the analytic constant-Cd flight without gravity", "[trajectory]")
{
	constexpr double DragCoefficient = 0.3;
	const FPchipCurve Curve = MakeCurve(DragCoefficient, DragCoefficient);
	FPointMassParams Params = MakeParams(Curve);
	Params.Gravity = FVector3::Zero();
	Params.Wind = FVector3::Zero();
	Params.EarthRotation = FVector3::Zero();
	const FPointMassModel Model(Params);

	constexpr double Duration = 1.5;
	const double Speed = Ogiva::Length(Muzzle.Velocity);
	const FVector3 Direction = Muzzle.Velocity / Speed;
	const double K = std::numbers::pi / 8.0 * AirDensity * DragCoefficient / BallisticCoefficient();
	const double Distance = std::log1p(K * Speed * Duration) / K;
	const double FinalSpeed = Speed / (1.0 + K * Speed * Duration);

	const FProjectileState Final = Fly(FDormandPrinceIntegrator{}, Model, Duration, 1500);

	CHECK_THAT(Ogiva::Length(Final.Position - Muzzle.Position), WithinRel(Distance, 1e-12));
	CHECK_THAT(Ogiva::Length(Final.Velocity), WithinRel(FinalSpeed, 1e-12));
	CHECK_THAT(Ogiva::Dot(Final.Velocity / Ogiva::Length(Final.Velocity), Direction), WithinRel(1.0, 1e-14));
}

TEST_CASE("Constant-Cd RK4 flight agrees with a tight Dormand-Prince reference", "[trajectory]")
{
	const FPchipCurve Curve = MakeCurve(0.3, 0.3);
	const FPointMassModel Model(MakeParams(Curve));
	constexpr double Duration = 1.5;

	const FProjectileState Reference = Fly(FDormandPrinceIntegrator{}, Model, Duration, 15000);
	const FProjectileState Rk4 = Fly(FRk4Integrator{}, Model, Duration, 1500);

	CHECK(Ogiva::Length(Rk4.Position - Reference.Position) < 1e-9);
	CHECK(Ogiva::Length(Rk4.Velocity - Reference.Velocity) < 1e-9);
}

TEST_CASE("RK4 global error falls as the fourth power of the step", "[trajectory]")
{
	const FPchipCurve Curve = MakeCurve(0.30, 0.20);
	const FPointMassModel Model(MakeParams(Curve));
	constexpr double Duration = 1.0;
	const FProjectileState Reference = Fly(FDormandPrinceIntegrator{}, Model, Duration, 10000);

	CHECK_THAT(ObservedOrder(FRk4Integrator{}, Model, Duration, 25, Reference), WithinAbs(4.0, 0.2));
}

TEST_CASE("Euler global error falls linearly with the step", "[trajectory]")
{
	const FPchipCurve Curve = MakeCurve(0.30, 0.20);
	const FPointMassModel Model(MakeParams(Curve));
	constexpr double Duration = 1.0;
	const FProjectileState Reference = Fly(FDormandPrinceIntegrator{}, Model, Duration, 10000);

	CHECK_THAT(ObservedOrder(FEulerIntegrator{}, Model, Duration, 400, Reference), WithinAbs(1.0, 0.1));
}
