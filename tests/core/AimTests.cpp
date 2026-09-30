#include "OgivaAim.h"
#include "OgivaAimFallback.h"

#include <catch_amalgamated.hpp>

#include <array>
#include <cmath>
#include <limits>

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;
using Ogiva::EError;
using Ogiva::EStepControl;
using Ogiva::FAimSolution;
using Ogiva::FDormandPrinceIntegrator;
using Ogiva::FFlightSettings;
using Ogiva::FHold;
using Ogiva::FPchipCurve;
using Ogiva::FPointMassModel;
using Ogiva::FPointMassParams;
using Ogiva::FRk4Integrator;
using Ogiva::FVector3;
using Ogiva::FZeroRequest;

namespace
{
	constexpr double G = Ogiva::StandardGravity;
	constexpr FFlightSettings Fixed{.StepControl = EStepControl::Fixed, .TimeStep = 0.002, .MaxTime = 60.0};
	constexpr FFlightSettings Adaptive{
		.StepControl = EStepControl::Adaptive,
		.TimeStep = 0.01,
		.MaxTime = 60.0,
		.AbsoluteTolerance = 1e-10,
		.RelativeTolerance = 1e-10,
	};
	constexpr FZeroRequest Rifle{
		.Muzzle = {.X = 0.0, .Y = 0.0, .Z = 1.5},
		.MuzzleSpeed = 850.0,
		.SightHeight = 0.05,
		.ZeroRange = 100.0,
	};

	const FPchipCurve& DragCurve()
	{
		static const FPchipCurve Curve = []
		{
			FPchipCurve Result;
			REQUIRE(FPchipCurve::Create(std::array{0.0, 5.0}, std::array{0.30, 0.20}, Result) == EError::None);
			return Result;
		}();
		return Curve;
	}

	FPointMassParams AirParams(const FVector3& Wind)
	{
		return {
			.DragCurve = &DragCurve(),
			.BallisticCoefficient = Ogiva::BallisticCoefficientFromImperial(0.25),
			.AirDensity = 1.225,
			.SpeedOfSound = 340.294,
			.Wind = Wind,
			.Gravity = {.X = 0.0, .Y = 0.0, .Z = -G},
			.EarthRotation = Ogiva::EarthRotationInFrame(-0.58, 0.4),
		};
	}

	FPointMassModel VacuumModel()
	{
		FPointMassParams Params = AirParams(FVector3::Zero());
		Params.AirDensity = 0.0;
		Params.EarthRotation = FVector3::Zero();
		return FPointMassModel(Params);
	}

	double LowVacuumElevation(double Speed, double Horizontal, double Height)
	{
		const double Speed2 = Speed * Speed;
		const double Discriminant = Speed2 * Speed2 - G * (G * Horizontal * Horizontal + 2.0 * Height * Speed2);
		return std::atan((Speed2 - std::sqrt(Discriminant)) / (G * Horizontal));
	}

	FAimSolution Aim(const Ogiva::IIntegrator& Integrator, const FPointMassModel& Model, const FVector3& Muzzle,
					 double Speed, const FVector3& Target, const FFlightSettings& Settings)
	{
		FAimSolution Solution;
		REQUIRE(Ogiva::SolveAim(Integrator, Model, Muzzle, Speed, Target, Settings, Solution) == EError::None);
		return Solution;
	}
}

TEST_CASE("LaunchDirection is a unit vector in the firing frame", "[aim]")
{
	const FVector3 Direction = Ogiva::LaunchDirection(0.3, -0.2);
	CHECK_THAT(Ogiva::Length(Direction), WithinRel(1.0, 1e-15));
	CHECK(Direction.Y < 0.0);
	CHECK(Direction.Z > 0.0);
	CHECK(Ogiva::LaunchDirection(0.0, 0.0) == FVector3{.X = 1.0, .Y = 0.0, .Z = 0.0});
}

TEST_CASE("A level vacuum aim matches the closed-form low angle", "[aim]")
{
	const FPointMassModel Model = VacuumModel();
	const FVector3 Target{.X = 300.0, .Y = 0.0, .Z = 0.0};
	const double Expected = 0.5 * std::asin(G * Target.X / (850.0 * 850.0));

	const FAimSolution FixedAim = Aim(FRk4Integrator{}, Model, FVector3::Zero(), 850.0, Target, Fixed);
	CHECK_THAT(FixedAim.Elevation, WithinRel(Expected, 1e-10));
	CHECK_THAT(FixedAim.Azimuth, WithinAbs(0.0, 1e-15));

	const FAimSolution AdaptiveAim = Aim(FDormandPrinceIntegrator{}, Model, FVector3::Zero(), 850.0, Target, Adaptive);
	CHECK_THAT(AdaptiveAim.Elevation, WithinRel(Expected, 1e-8));
	CHECK(Ogiva::Length(AdaptiveAim.Crossing.State.Position - Target) < 1e-7);
}

TEST_CASE("An inclined vacuum aim matches the closed-form low angle and bearing", "[aim]")
{
	const FPointMassModel Model = VacuumModel();
	const FVector3 Muzzle{.X = 10.0, .Y = -5.0, .Z = 2.0};
	const FVector3 Target{.X = 410.0, .Y = 45.0, .Z = 62.0};
	const FVector3 Offset = Target - Muzzle;
	const double Horizontal = std::hypot(Offset.X, Offset.Y);

	const FAimSolution Solution = Aim(FRk4Integrator{}, Model, Muzzle, 850.0, Target, Fixed);

	CHECK_THAT(Solution.Elevation, WithinRel(LowVacuumElevation(850.0, Horizontal, Offset.Z), 1e-10));
	CHECK_THAT(Solution.Azimuth, WithinRel(std::atan2(Offset.Y, Offset.X), 1e-12));
}

TEST_CASE("A vacuum aim near maximum range takes the low angle", "[aim]")
{
	const FPointMassModel Model = VacuumModel();
	constexpr double Speed = 100.0;
	const FVector3 Target{.X = 1000.0, .Y = 0.0, .Z = 0.0};

	const FAimSolution Solution = Aim(FDormandPrinceIntegrator{}, Model, FVector3::Zero(), Speed, Target, Adaptive);

	CHECK_THAT(Solution.Elevation, WithinRel(LowVacuumElevation(Speed, Target.X, 0.0), 1e-7));
}

TEST_CASE("A target beyond maximum range is out of range", "[aim]")
{
	FAimSolution Solution;
	const FVector3 Target{.X = 1100.0, .Y = 0.0, .Z = 0.0};

	CHECK(Ogiva::SolveAim(FDormandPrinceIntegrator{}, VacuumModel(), FVector3::Zero(), 100.0, Target, Adaptive,
						  Solution) == EError::OutOfRange);
	CHECK(Ogiva::SolveAim(FDormandPrinceIntegrator{}, FPointMassModel(AirParams(FVector3::Zero())), FVector3::Zero(),
						  100.0, {.X = 900.0, .Y = 0.0, .Z = 0.0}, Adaptive, Solution) == EError::OutOfRange);
}

TEST_CASE("An aim in wind, drag and Earth rotation hits the target", "[aim]")
{
	const FPointMassModel Model(AirParams({.X = -2.0, .Y = 4.0, .Z = 0.0}));
	const FVector3 Muzzle{.X = 0.0, .Y = 0.0, .Z = 1.5};
	const FVector3 Target{.X = 800.0, .Y = -3.0, .Z = 20.0};

	const FAimSolution AdaptiveAim = Aim(FDormandPrinceIntegrator{}, Model, Muzzle, 850.0, Target, Adaptive);
	CHECK(Ogiva::Length(AdaptiveAim.Crossing.State.Position - Target) < 1e-7);

	const FAimSolution FixedAim = Aim(FRk4Integrator{}, Model, Muzzle, 850.0, Target, Fixed);
	CHECK(Ogiva::Length(FixedAim.Crossing.State.Position - Target) < 1e-9);
	CHECK_THAT(FixedAim.Elevation, WithinAbs(AdaptiveAim.Elevation, 1e-9));
	CHECK_THAT(FixedAim.Azimuth, WithinAbs(AdaptiveAim.Azimuth, 1e-9));
}

TEST_CASE("SolveAim rejects invalid inputs", "[aim]")
{
	const FPointMassModel Model = VacuumModel();
	const FVector3 Target{.X = 300.0, .Y = 0.0, .Z = 0.0};
	constexpr double NaN = std::numeric_limits<double>::quiet_NaN();
	FAimSolution Solution;
	const FRk4Integrator Rk4;

	CHECK(Ogiva::SolveAim(Rk4, Model, FVector3::Zero(), 0.0, Target, Fixed, Solution) == EError::InvalidArgument);
	CHECK(Ogiva::SolveAim(Rk4, Model, FVector3::Zero(), NaN, Target, Fixed, Solution) == EError::InvalidArgument);
	CHECK(Ogiva::SolveAim(Rk4, Model, FVector3::Zero(), 850.0, FVector3::Zero(), Fixed, Solution) ==
		  EError::InvalidArgument);
	CHECK(Ogiva::SolveAim(Rk4, Model, FVector3::Zero(), 850.0, {.X = 0.0, .Y = 0.0, .Z = 50.0}, Fixed, Solution) ==
		  EError::InvalidArgument);
	CHECK(Ogiva::SolveAim(Rk4, Model, FVector3::Zero(), 850.0, {.X = NaN, .Y = 0.0, .Z = 0.0}, Fixed, Solution) ==
		  EError::InvalidArgument);
	CHECK(Ogiva::SolveAim(Rk4, Model, FVector3::Zero(), 850.0, Target, Adaptive, Solution) == EError::InvalidArgument);
}

TEST_CASE("A vacuum zero matches the closed form through the sight point", "[aim]")
{
	FAimSolution Zero;
	REQUIRE(Ogiva::SolveZero(FRk4Integrator{}, VacuumModel(), Rifle, Fixed, Zero) == EError::None);

	CHECK_THAT(Zero.Elevation,
			   WithinRel(LowVacuumElevation(Rifle.MuzzleSpeed, Rifle.ZeroRange, Rifle.SightHeight), 1e-10));
	CHECK_THAT(Zero.Azimuth, WithinAbs(0.0, 1e-15));
}

TEST_CASE("Vacuum holds match the closed-form drop and angle", "[aim]")
{
	const FPointMassModel Model = VacuumModel();
	const FRk4Integrator Rk4;
	FAimSolution Zero;
	REQUIRE(Ogiva::SolveZero(Rk4, Model, Rifle, Fixed, Zero) == EError::None);

	FHold AtZero;
	REQUIRE(Ogiva::SolveHold(Rk4, Model, Rifle, Zero, Rifle.ZeroRange, Fixed, AtZero) == EError::None);
	CHECK_THAT(AtZero.Drop, WithinAbs(0.0, 1e-9));
	CHECK_THAT(AtZero.Elevation, WithinAbs(0.0, 1e-12));

	constexpr double Range = 400.0;
	const double Speed = Rifle.MuzzleSpeed * std::cos(Zero.Elevation);
	const double Height = Range * std::tan(Zero.Elevation) - G * Range * Range / (2.0 * Speed * Speed);
	FHold Hold;
	REQUIRE(Ogiva::SolveHold(Rk4, Model, Rifle, Zero, Range, Fixed, Hold) == EError::None);
	CHECK_THAT(Hold.Drop, WithinRel(Rifle.SightHeight - Height, 1e-9));
	CHECK_THAT(Hold.Elevation,
			   WithinRel(LowVacuumElevation(Rifle.MuzzleSpeed, Range, Rifle.SightHeight) - Zero.Elevation, 1e-8));
	CHECK_THAT(Hold.Drift, WithinAbs(0.0, 1e-12));
	CHECK_THAT(Hold.Windage, WithinAbs(0.0, 1e-15));
}

TEST_CASE("A crosswind toward the left drifts left and holds right", "[aim]")
{
	const FPointMassModel Calm(AirParams(FVector3::Zero()));
	const FPointMassModel Windy(AirParams({.X = 0.0, .Y = 5.0, .Z = 0.0}));
	FAimSolution Zero;
	REQUIRE(Ogiva::SolveZero(FDormandPrinceIntegrator{}, Calm, Rifle, Adaptive, Zero) == EError::None);

	FHold Calmed;
	FHold Hold;
	REQUIRE(Ogiva::SolveHold(FDormandPrinceIntegrator{}, Calm, Rifle, Zero, 600.0, Adaptive, Calmed) == EError::None);
	REQUIRE(Ogiva::SolveHold(FDormandPrinceIntegrator{}, Windy, Rifle, Zero, 600.0, Adaptive, Hold) == EError::None);

	CHECK(Hold.Drift > Calmed.Drift + 0.1);
	CHECK(Hold.Windage < Calmed.Windage);
	CHECK(Hold.Drop > 0.0);
	CHECK(Hold.Elevation > 0.0);
}

TEST_CASE("SolveZero and SolveHold reject invalid requests", "[aim]")
{
	const FPointMassModel Model = VacuumModel();
	const FRk4Integrator Rk4;
	FZeroRequest Request = Rifle;
	Request.ZeroRange = 0.0;
	FAimSolution Zero;
	CHECK(Ogiva::SolveZero(Rk4, Model, Request, Fixed, Zero) == EError::InvalidArgument);

	REQUIRE(Ogiva::SolveZero(Rk4, Model, Rifle, Fixed, Zero) == EError::None);
	FHold Hold;
	CHECK(Ogiva::SolveHold(Rk4, Model, Rifle, Zero, -5.0, Fixed, Hold) == EError::InvalidArgument);
	Zero.Elevation = std::numeric_limits<double>::quiet_NaN();
	CHECK(Ogiva::SolveHold(Rk4, Model, Rifle, Zero, 300.0, Fixed, Hold) == EError::InvalidArgument);
}

TEST_CASE("The bracketing fallback matches the closed-form vacuum aim", "[aim]")
{
	const FVector3 Target{.X = 300.0, .Y = 20.0, .Z = 15.0};
	FAimSolution Solution;

	REQUIRE(Ogiva::AimFallback::SolveByBracketing(FRk4Integrator{}, VacuumModel(), FVector3::Zero(), 850.0, Target,
												  Fixed, Solution) == EError::None);

	CHECK_THAT(Solution.Elevation,
			   WithinRel(LowVacuumElevation(850.0, std::hypot(Target.X, Target.Y), Target.Z), 1e-10));
	CHECK_THAT(Solution.Azimuth, WithinRel(std::atan2(Target.Y, Target.X), 1e-12));
}

TEST_CASE("The bracketing fallback agrees with Broyden in wind and near maximum range", "[aim]")
{
	const FPointMassModel Model(AirParams({.X = 0.0, .Y = 6.0, .Z = 0.0}));
	const FDormandPrinceIntegrator DormandPrince;
	constexpr double Speed = 100.0;

	for (const FVector3& Target :
		 std::array{FVector3{.X = 400.0, .Y = 10.0, .Z = 5.0}, FVector3{.X = 645.0, .Y = 0.0, .Z = 0.0}})
	{
		const FAimSolution Broyden = Aim(DormandPrince, Model, FVector3::Zero(), Speed, Target, Adaptive);
		FAimSolution Fallback;
		REQUIRE(Ogiva::AimFallback::SolveByBracketing(DormandPrince, Model, FVector3::Zero(), Speed, Target, Adaptive,
													  Fallback) == EError::None);

		CHECK(Ogiva::Length(Fallback.Crossing.State.Position - Target) < 1e-7);
		CHECK_THAT(Fallback.Elevation, WithinAbs(Broyden.Elevation, 1e-9));
		CHECK_THAT(Fallback.Azimuth, WithinAbs(Broyden.Azimuth, 1e-9));
	}
}
