#include "OgivaDrag.h"
#include "OgivaPointMass.h"

#include <catch_amalgamated.hpp>

#include <cmath>
#include <numbers>

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;
using Ogiva::EDragModel;
using Ogiva::FPointMassModel;
using Ogiva::FPointMassParams;
using Ogiva::FProjectileState;
using Ogiva::FVector3;

namespace
{
	constexpr FVector3 StandardGravityVector{.X = 0.0, .Y = 0.0, .Z = -Ogiva::StandardGravity};

	FPointMassParams MakeAirParams()
	{
		return {
			.DragCurve = &Ogiva::GetReferenceDragCurve(EDragModel::G7),
			.BallisticCoefficient = Ogiva::BallisticCoefficientFromImperial(0.25),
			.AirDensity = 1.225,
			.SpeedOfSound = 340.294,
			.Wind = FVector3::Zero(),
			.Gravity = StandardGravityVector,
			.EarthRotation = FVector3::Zero(),
		};
	}

	void CheckVectorWithinRel(const FVector3& Actual, const FVector3& Expected, double Tolerance)
	{
		CHECK_THAT(Actual.X, WithinRel(Expected.X, Tolerance));
		CHECK_THAT(Actual.Y, WithinRel(Expected.Y, Tolerance));
		CHECK_THAT(Actual.Z, WithinRel(Expected.Z, Tolerance));
	}
}

TEST_CASE("Without air or rotation the acceleration is gravity alone", "[pointmass]")
{
	FPointMassParams Params = MakeAirParams();
	Params.AirDensity = 0.0;
	const FPointMassModel Model(Params);
	const FProjectileState State{
		.Position = {.X = 10.0, .Y = 0.5, .Z = 1.5},
		.Velocity = {.X = 850.0, .Y = -3.0, .Z = 12.0},
	};

	const Ogiva::FStateDerivative Derivative = Model.Derivative(0.0, State);

	CHECK(Derivative.Velocity == State.Velocity);
	CHECK(Derivative.Acceleration == StandardGravityVector);
}

TEST_CASE("Drag matches the PRD form in mass, diameter and form factor", "[pointmass]")
{
	constexpr double Mass = 0.0113;
	constexpr double Diameter = 0.00782;
	constexpr double FormFactor = 0.93;
	constexpr double AirDensity = 1.1;
	constexpr double SpeedOfSound = 335.0;

	FPointMassParams Params = MakeAirParams();
	Params.BallisticCoefficient = Mass / (FormFactor * Diameter * Diameter);
	Params.AirDensity = AirDensity;
	Params.SpeedOfSound = SpeedOfSound;
	Params.Gravity = FVector3::Zero();
	const FPointMassModel Model(Params);

	for (const double Speed : {150.0, 330.0, 345.0, 600.0, 900.0})
	{
		const FVector3 Velocity{.X = Speed * 0.8, .Y = Speed * 0.36, .Z = Speed * 0.48};
		const double ReferenceCd = Ogiva::ReferenceDragCoefficient(EDragModel::G7, Speed / SpeedOfSound);
		const FVector3 Expected =
			-(AirDensity * std::numbers::pi * Diameter * Diameter * FormFactor * ReferenceCd / (8.0 * Mass) * Speed) *
			Velocity;

		CheckVectorWithinRel(Model.Derivative(0.0, {.Position = FVector3::Zero(), .Velocity = Velocity}).Acceleration,
							 Expected, 1e-13);
	}
}

TEST_CASE("Drag acts on velocity relative to the air", "[pointmass]")
{
	FPointMassParams Params = MakeAirParams();
	Params.Gravity = FVector3::Zero();
	Params.Wind = {.X = -4.0, .Y = 6.0, .Z = 0.0};

	const FProjectileState MovingWithAir{.Position = FVector3::Zero(), .Velocity = Params.Wind};
	CHECK(FPointMassModel(Params).Derivative(0.0, MovingWithAir).Acceleration == FVector3::Zero());

	const FProjectileState Shot{.Position = FVector3::Zero(), .Velocity = {.X = 800.0, .Y = 0.0, .Z = 0.0}};
	const FVector3 Acceleration = FPointMassModel(Params).Derivative(0.0, Shot).Acceleration;
	CHECK(Acceleration.X < 0.0);
	CHECK(Acceleration.Y > 0.0);
	CHECK_THAT(Acceleration.Y / Acceleration.X, WithinRel(-6.0 / 804.0, 1e-14));
}

TEST_CASE("Derivative does not depend on time in M1", "[pointmass]")
{
	const FPointMassModel Model(MakeAirParams());
	const FProjectileState State{.Position = FVector3::Zero(), .Velocity = {.X = 700.0, .Y = 1.0, .Z = 2.0}};
	CHECK(Model.Derivative(0.0, State).Acceleration == Model.Derivative(1.75, State).Acceleration);
}

TEST_CASE("A missing drag curve yields NaN instead of silently dropping drag", "[pointmass]")
{
	FPointMassParams Params = MakeAirParams();
	Params.DragCurve = nullptr;
	const FVector3 Acceleration =
		FPointMassModel(Params)
			.Derivative(0.0, {.Position = FVector3::Zero(), .Velocity = {.X = 800.0, .Y = 0.0, .Z = 0.0}})
			.Acceleration;
	CHECK(std::isnan(Acceleration.X));
}

TEST_CASE("Ballistic coefficient converts from lb/in2 to kg/m2", "[pointmass]")
{
	CHECK_THAT(Ogiva::BallisticCoefficientFromImperial(1.0), WithinRel(703.0695796391, 1e-12));
	CHECK_THAT(Ogiva::BallisticCoefficientFromImperial(0.243), WithinRel(0.243 * 703.0695796391, 1e-12));
}

TEST_CASE("Earth rotation is expressed in the downrange, left, up frame", "[pointmass]")
{
	constexpr double Omega = Ogiva::EarthRotationRate;
	constexpr double HalfPi = std::numbers::pi / 2.0;

	const FVector3 NorthAtEquator = Ogiva::EarthRotationInFrame(0.0, 0.0);
	CHECK(NorthAtEquator == FVector3{.X = Omega, .Y = 0.0, .Z = 0.0});

	const FVector3 EastAtEquator = Ogiva::EarthRotationInFrame(0.0, HalfPi);
	CHECK_THAT(EastAtEquator.X, WithinAbs(0.0, 1e-20));
	CHECK_THAT(EastAtEquator.Y, WithinRel(Omega, 1e-15));
	CHECK(EastAtEquator.Z == 0.0);

	const FVector3 NorthPole = Ogiva::EarthRotationInFrame(HalfPi, 1.0);
	CHECK_THAT(NorthPole.Z, WithinRel(Omega, 1e-15));

	CHECK_THAT(Ogiva::Length(Ogiva::EarthRotationInFrame(-0.58, 2.3)), WithinRel(Omega, 1e-15));
}

TEST_CASE("Coriolis deflects right in the northern hemisphere and lifts eastward shots", "[pointmass]")
{
	FPointMassParams Params = MakeAirParams();
	Params.AirDensity = 0.0;
	Params.Gravity = FVector3::Zero();
	const FProjectileState Shot{.Position = FVector3::Zero(), .Velocity = {.X = 800.0, .Y = 0.0, .Z = 0.0}};

	Params.EarthRotation = Ogiva::EarthRotationInFrame(0.7, 0.0);
	CHECK(FPointMassModel(Params).Derivative(0.0, Shot).Acceleration.Y < 0.0);

	Params.EarthRotation = Ogiva::EarthRotationInFrame(-0.7, 0.0);
	CHECK(FPointMassModel(Params).Derivative(0.0, Shot).Acceleration.Y > 0.0);

	Params.EarthRotation = Ogiva::EarthRotationInFrame(0.0, std::numbers::pi / 2.0);
	const FVector3 Eastward = FPointMassModel(Params).Derivative(0.0, Shot).Acceleration;
	CHECK_THAT(Eastward.Z, WithinRel(2.0 * Ogiva::EarthRotationRate * 800.0, 1e-15));
}
