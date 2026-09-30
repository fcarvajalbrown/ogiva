#include "OgivaFlight.h"

#include <catch_amalgamated.hpp>

#include <array>
#include <cmath>
#include <limits>

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;
using Ogiva::EError;
using Ogiva::EStepControl;
using Ogiva::FDormandPrinceIntegrator;
using Ogiva::FEulerIntegrator;
using Ogiva::FFlightSettings;
using Ogiva::FPchipCurve;
using Ogiva::FPlane;
using Ogiva::FPlaneCrossing;
using Ogiva::FPointMassModel;
using Ogiva::FPointMassParams;
using Ogiva::FProjectileState;
using Ogiva::FRk4Integrator;
using Ogiva::FVector3;
using Ogiva::IIntegrator;

namespace
{
	constexpr FVector3 Gravity{.X = 0.0, .Y = 0.0, .Z = -Ogiva::StandardGravity};
	constexpr FProjectileState Muzzle{
		.Position = {.X = 0.0, .Y = 0.0, .Z = 1.5},
		.Velocity = {.X = 850.0, .Y = 1.0, .Z = 8.0},
	};
	constexpr FPlane Ground{.Normal = FVector3::Up(), .Offset = 0.0};
	constexpr FPlane Downrange500{.Normal = {.X = 1.0, .Y = 0.0, .Z = 0.0}, .Offset = 500.0};
	constexpr FFlightSettings Fixed{.StepControl = EStepControl::Fixed, .TimeStep = 0.01, .MaxTime = 10.0};
	constexpr FFlightSettings Adaptive{
		.StepControl = EStepControl::Adaptive,
		.TimeStep = 0.01,
		.MaxTime = 10.0,
		.AbsoluteTolerance = 1e-10,
		.RelativeTolerance = 1e-10,
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

	FPointMassParams AirParams()
	{
		return {
			.DragCurve = &DragCurve(),
			.BallisticCoefficient = Ogiva::BallisticCoefficientFromImperial(0.25),
			.AirDensity = 1.225,
			.SpeedOfSound = 340.294,
			.Wind = {.X = -2.0, .Y = 3.0, .Z = 0.0},
			.Gravity = Gravity,
			.EarthRotation = Ogiva::EarthRotationInFrame(-0.58, 0.4),
		};
	}

	FPointMassModel VacuumModel()
	{
		FPointMassParams Params = AirParams();
		Params.AirDensity = 0.0;
		Params.EarthRotation = FVector3::Zero();
		return FPointMassModel(Params);
	}

	FPlaneCrossing Fly(const IIntegrator& Integrator, const FPointMassModel& Model, const FPlane& Plane,
					   const FFlightSettings& Settings)
	{
		FPlaneCrossing Crossing;
		REQUIRE(Ogiva::FlyToPlane(Integrator, Model, Muzzle, Plane, Settings, Crossing) == EError::None);
		return Crossing;
	}

	EError FlyStatus(const IIntegrator& Integrator, const FPlane& Plane, const FFlightSettings& Settings,
					 const FProjectileState& Initial = Muzzle)
	{
		FPlaneCrossing Crossing;
		return Ogiva::FlyToPlane(Integrator, VacuumModel(), Initial, Plane, Settings, Crossing);
	}

	void CheckOnParabola(const FPlaneCrossing& Crossing, double Time)
	{
		const double T = Crossing.Time;
		const FVector3 Position = Muzzle.Position + Muzzle.Velocity * T + Gravity * (0.5 * T * T);
		const FVector3 Velocity = Muzzle.Velocity + Gravity * T;
		CHECK_THAT(T, WithinRel(Time, 1e-13));
		CHECK_THAT(Crossing.State.Position.X, WithinRel(Position.X, 1e-13));
		CHECK_THAT(Crossing.State.Position.Y, WithinRel(Position.Y, 1e-13));
		CHECK_THAT(Crossing.State.Position.Z, WithinAbs(Position.Z, 1e-11));
		CHECK_THAT(Crossing.State.Velocity.Z, WithinRel(Velocity.Z, 1e-13));
	}
}

TEST_CASE("The vacuum ground crossing matches the closed-form impact time", "[flight]")
{
	const double A = 0.5 * Ogiva::StandardGravity;
	const double ImpactTime =
		(Muzzle.Velocity.Z + std::sqrt(Muzzle.Velocity.Z * Muzzle.Velocity.Z + 4.0 * A * Muzzle.Position.Z)) /
		(2.0 * A);
	const FPointMassModel Model = VacuumModel();

	const FPlaneCrossing FixedCrossing = Fly(FRk4Integrator{}, Model, Ground, Fixed);
	CheckOnParabola(FixedCrossing, ImpactTime);
	CHECK_THAT(FixedCrossing.State.Position.Z, WithinAbs(0.0, 1e-11));

	const FPlaneCrossing AdaptiveCrossing = Fly(FDormandPrinceIntegrator{}, Model, Ground, Adaptive);
	CheckOnParabola(AdaptiveCrossing, ImpactTime);
	CHECK_THAT(AdaptiveCrossing.State.Position.Z, WithinAbs(0.0, 1e-11));
}

TEST_CASE("The vacuum downrange crossing matches the closed-form flight time", "[flight]")
{
	const FPointMassModel Model = VacuumModel();
	const double FlightTime = Downrange500.Offset / Muzzle.Velocity.X;

	CheckOnParabola(Fly(FRk4Integrator{}, Model, Downrange500, Fixed), FlightTime);
	CheckOnParabola(Fly(FDormandPrinceIntegrator{}, Model, Downrange500, Adaptive), FlightTime);
}

TEST_CASE("An inclined plane crossing lies on the plane", "[flight]")
{
	const FPointMassModel Model(AirParams());
	const FPlane Inclined{.Normal = {.X = 0.8, .Y = 0.1, .Z = -0.6}, .Offset = 250.0};

	const std::array Crossings{
		Fly(FRk4Integrator{}, Model, Inclined, Fixed),
		Fly(FDormandPrinceIntegrator{}, Model, Inclined, Adaptive),
	};
	for (const FPlaneCrossing& Crossing : Crossings)
	{
		CHECK_THAT(Ogiva::Dot(Inclined.Normal, Crossing.State.Position), WithinRel(Inclined.Offset, 1e-14));
	}
}

TEST_CASE("Adaptive Dormand-Prince agrees with fine fixed-step RK4 in air", "[flight]")
{
	const FPointMassModel Model(AirParams());
	const FPlane Downrange800{.Normal = {.X = 1.0, .Y = 0.0, .Z = 0.0}, .Offset = 800.0};
	FFlightSettings Fine = Fixed;
	Fine.TimeStep = 1e-4;

	const FPlaneCrossing Reference = Fly(FRk4Integrator{}, Model, Downrange800, Fine);
	const FPlaneCrossing Crossing = Fly(FDormandPrinceIntegrator{}, Model, Downrange800, Adaptive);

	CHECK_THAT(Crossing.Time, WithinAbs(Reference.Time, 1e-10));
	CHECK(Ogiva::Length(Crossing.State.Position - Reference.State.Position) < 1e-7);
	CHECK(Ogiva::Length(Crossing.State.Velocity - Reference.State.Velocity) < 1e-7);
}

TEST_CASE("Adaptive step control needs an integrator with an error estimate", "[flight]")
{
	CHECK(FlyStatus(FRk4Integrator{}, Ground, Adaptive) == EError::InvalidArgument);
	CHECK(FlyStatus(FEulerIntegrator{}, Ground, Adaptive) == EError::InvalidArgument);
}

TEST_CASE("FlyToPlane rejects invalid planes, settings and states", "[flight]")
{
	const FRk4Integrator Rk4;
	const FDormandPrinceIntegrator DormandPrince;
	constexpr double NaN = std::numeric_limits<double>::quiet_NaN();

	CHECK(FlyStatus(Rk4, {.Normal = FVector3::Zero(), .Offset = 0.0}, Fixed) == EError::InvalidArgument);
	CHECK(FlyStatus(Rk4, {.Normal = {.X = NaN, .Y = 0.0, .Z = 1.0}, .Offset = 0.0}, Fixed) == EError::InvalidArgument);
	CHECK(FlyStatus(Rk4, {.Normal = FVector3::Up(), .Offset = NaN}, Fixed) == EError::InvalidArgument);

	FFlightSettings Settings = Fixed;
	Settings.TimeStep = 0.0;
	CHECK(FlyStatus(Rk4, Ground, Settings) == EError::InvalidArgument);
	Settings = Fixed;
	Settings.MaxTime = -1.0;
	CHECK(FlyStatus(Rk4, Ground, Settings) == EError::InvalidArgument);
	Settings = Fixed;
	Settings.StepControl = static_cast<EStepControl>(7);
	CHECK(FlyStatus(Rk4, Ground, Settings) == EError::InvalidArgument);

	Settings = Adaptive;
	Settings.AbsoluteTolerance = 0.0;
	CHECK(FlyStatus(DormandPrince, Ground, Settings) == EError::InvalidArgument);
	Settings = Adaptive;
	Settings.RelativeTolerance = -1e-9;
	CHECK(FlyStatus(DormandPrince, Ground, Settings) == EError::InvalidArgument);

	FProjectileState Initial = Muzzle;
	Initial.Velocity.X = NaN;
	CHECK(FlyStatus(Rk4, Ground, Fixed, Initial) == EError::InvalidArgument);
}

TEST_CASE("A plane beyond MaxTime is not reached", "[flight]")
{
	const FPlane Far{.Normal = {.X = 1.0, .Y = 0.0, .Z = 0.0}, .Offset = 5000.0};
	FFlightSettings Settings = Fixed;
	Settings.MaxTime = 1.0;
	CHECK(FlyStatus(FRk4Integrator{}, Far, Settings) == EError::TargetNotReached);

	Settings = Adaptive;
	Settings.MaxTime = 1.0;
	CHECK(FlyStatus(FDormandPrinceIntegrator{}, Far, Settings) == EError::TargetNotReached);
}

TEST_CASE("A state already on the plane crosses at time zero", "[flight]")
{
	const FPlane MuzzlePlane{.Normal = FVector3::Up(), .Offset = Muzzle.Position.Z};
	FPlaneCrossing Crossing{.Time = -1.0, .State = {}};

	REQUIRE(Ogiva::FlyToPlane(FRk4Integrator{}, VacuumModel(), Muzzle, MuzzlePlane, Fixed, Crossing) == EError::None);
	CHECK(Crossing.Time == 0.0);
	CHECK(Crossing.State == Muzzle);
}

TEST_CASE("A model that yields NaN stops the flight", "[flight]")
{
	FPointMassParams Params = AirParams();
	Params.DragCurve = nullptr;
	const FPointMassModel Model(Params);
	FPlaneCrossing Crossing;

	CHECK(Ogiva::FlyToPlane(FRk4Integrator{}, Model, Muzzle, Ground, Fixed, Crossing) == EError::NotConverged);
	CHECK(Ogiva::FlyToPlane(FDormandPrinceIntegrator{}, Model, Muzzle, Ground, Adaptive, Crossing) ==
		  EError::NotConverged);
}
