#include "OgivaDrag.h"
#include "OgivaIntegrator.h"

#include <catch_amalgamated.hpp>

#include <array>
#include <cmath>
#include <vector>

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;
using Ogiva::EError;
using Ogiva::FDormandPrinceIntegrator;
using Ogiva::FEulerIntegrator;
using Ogiva::FPchipCurve;
using Ogiva::FPointMassModel;
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
		.Velocity = {.X = 850.0, .Y = 2.0, .Z = 6.0},
	};

	FPointMassModel MakeVacuumModel()
	{
		return FPointMassModel({
			.DragCurve = &Ogiva::GetReferenceDragCurve(Ogiva::EDragModel::G7),
			.BallisticCoefficient = Ogiva::BallisticCoefficientFromImperial(0.25),
			.AirDensity = 0.0,
			.SpeedOfSound = 340.294,
			.Wind = FVector3::Zero(),
			.Gravity = Gravity,
			.EarthRotation = FVector3::Zero(),
		});
	}

	const FPchipCurve& SmoothDragCurve()
	{
		static const FPchipCurve Curve = []
		{
			FPchipCurve Result;
			REQUIRE(FPchipCurve::Create(std::array{0.0, 5.0}, std::array{0.30, 0.20}, Result) == EError::None);
			return Result;
		}();
		return Curve;
	}

	FPointMassModel MakeSmoothAirModel()
	{
		return FPointMassModel({
			.DragCurve = &SmoothDragCurve(),
			.BallisticCoefficient = Ogiva::BallisticCoefficientFromImperial(0.25),
			.AirDensity = 1.225,
			.SpeedOfSound = 340.294,
			.Wind = {.X = -3.0, .Y = 4.0, .Z = 0.0},
			.Gravity = Gravity,
			.EarthRotation = Ogiva::EarthRotationInFrame(-0.58, 0.4),
		});
	}

	FStepResult StepOne(const IIntegrator& Integrator, const FPointMassModel& Model, double Time,
						const FProjectileState& State, double TimeStep)
	{
		std::array<FStepResult, 1> Result{};
		REQUIRE(Integrator.StepBatch(Model, Time, std::span(&State, 1), TimeStep, Result) == EError::None);
		return Result.front();
	}

	FProjectileState Reference(const FPointMassModel& Model, const FProjectileState& State, double TimeStep)
	{
		constexpr int SubSteps = 1000;
		const FDormandPrinceIntegrator Integrator;
		const double SubStep = TimeStep / SubSteps;
		FProjectileState Current = State;
		for (int Index = 0; Index < SubSteps; ++Index)
		{
			Current = StepOne(Integrator, Model, Index * SubStep, Current, SubStep).State;
		}
		return Current;
	}

	double VelocityErrorNorm(const FProjectileState& Actual, const FProjectileState& Expected)
	{
		return Ogiva::Length(Actual.Velocity - Expected.Velocity);
	}

	void CheckExactParabola(const FProjectileState& Actual, double TimeStep)
	{
		const FVector3 Position = Muzzle.Position + Muzzle.Velocity * TimeStep + Gravity * (0.5 * TimeStep * TimeStep);
		const FVector3 Velocity = Muzzle.Velocity + Gravity * TimeStep;
		CHECK_THAT(Actual.Position.X, WithinRel(Position.X, 1e-15));
		CHECK_THAT(Actual.Position.Y, WithinRel(Position.Y, 1e-15));
		CHECK_THAT(Actual.Position.Z, WithinRel(Position.Z, 1e-14));
		CHECK_THAT(Actual.Velocity.Z, WithinRel(Velocity.Z, 1e-14));
		CHECK(Actual.Velocity.X == Velocity.X);
	}
}

TEST_CASE("StepBatch rejects mismatched spans", "[integrator]")
{
	const FPointMassModel Model = MakeVacuumModel();
	const std::array States{Muzzle, Muzzle};
	std::array<FStepResult, 1> Results{};

	CHECK(FEulerIntegrator{}.StepBatch(Model, 0.0, States, 0.01, Results) == EError::InvalidArgument);
	CHECK(FRk4Integrator{}.StepBatch(Model, 0.0, States, 0.01, Results) == EError::InvalidArgument);
	CHECK(FDormandPrinceIntegrator{}.StepBatch(Model, 0.0, States, 0.01, Results) == EError::InvalidArgument);
}

TEST_CASE("Euler takes one explicit step in vacuum", "[integrator]")
{
	constexpr double TimeStep = 0.01;
	const FStepResult Result = StepOne(FEulerIntegrator{}, MakeVacuumModel(), 0.0, Muzzle, TimeStep);

	CHECK(Result.State.Position == Muzzle.Position + Muzzle.Velocity * TimeStep);
	CHECK(Result.State.Velocity == Muzzle.Velocity + Gravity * TimeStep);
}

TEST_CASE("RK4 and Dormand-Prince reproduce the vacuum parabola in one step", "[integrator]")
{
	constexpr double TimeStep = 0.25;
	const FPointMassModel Model = MakeVacuumModel();

	CheckExactParabola(StepOne(FRk4Integrator{}, Model, 0.0, Muzzle, TimeStep).State, TimeStep);

	const FStepResult DormandPrince = StepOne(FDormandPrinceIntegrator{}, Model, 0.0, Muzzle, TimeStep);
	CheckExactParabola(DormandPrince.State, TimeStep);
	CHECK_THAT(Ogiva::Length(DormandPrince.LocalError.Position), WithinAbs(0.0, 1e-12));
	CHECK_THAT(Ogiva::Length(DormandPrince.LocalError.Velocity), WithinAbs(0.0, 1e-12));
}

TEST_CASE("Every integrator reports the derivative at the end of its step", "[integrator]")
{
	constexpr double Time = 0.3;
	constexpr double TimeStep = 0.01;
	const FPointMassModel Model = MakeSmoothAirModel();
	const FEulerIntegrator Euler;
	const FRk4Integrator Rk4;
	const FDormandPrinceIntegrator DormandPrince;
	const std::array<const IIntegrator*, 3> Integrators{&Euler, &Rk4, &DormandPrince};

	for (const IIntegrator* Integrator : Integrators)
	{
		const FStepResult Result = StepOne(*Integrator, Model, Time, Muzzle, TimeStep);
		const Ogiva::FStateDerivative Expected = Model.Derivative(Time + TimeStep, Result.State);
		CHECK(Result.EndDerivative.Velocity == Expected.Velocity);
		CHECK(Result.EndDerivative.Acceleration == Expected.Acceleration);
	}
}

TEST_CASE("Fixed-step integrators report zero local error", "[integrator]")
{
	const FPointMassModel Model = MakeSmoothAirModel();
	const FProjectileState Zero{.Position = FVector3::Zero(), .Velocity = FVector3::Zero()};

	CHECK(StepOne(FEulerIntegrator{}, Model, 0.0, Muzzle, 0.01).LocalError == Zero);
	CHECK(StepOne(FRk4Integrator{}, Model, 0.0, Muzzle, 0.01).LocalError == Zero);
}

TEST_CASE("A batch gives the same results as stepping each state alone", "[integrator]")
{
	const FPointMassModel Model = MakeSmoothAirModel();
	const std::vector<FProjectileState> States{
		Muzzle,
		{.Position = {.X = 100.0, .Y = 0.2, .Z = 1.4}, .Velocity = {.X = 700.0, .Y = 1.0, .Z = 3.0}},
		{.Position = {.X = 400.0, .Y = -0.5, .Z = 0.3}, .Velocity = {.X = 300.0, .Y = -2.0, .Z = -4.0}},
	};
	std::vector<FStepResult> Results(States.size());
	const FDormandPrinceIntegrator Integrator;

	REQUIRE(Integrator.StepBatch(Model, 0.1, States, 0.005, Results) == EError::None);

	auto Result = Results.begin();
	for (const FProjectileState& State : States)
	{
		const FStepResult Single = StepOne(Integrator, Model, 0.1, State, 0.005);
		CHECK(Result->State == Single.State);
		CHECK(Result->LocalError == Single.LocalError);
		++Result;
	}
}

TEST_CASE("RK4 local error falls as the fifth power of the step", "[integrator]")
{
	const FPointMassModel Model = MakeSmoothAirModel();
	constexpr double CoarseStep = 0.05;
	constexpr double FineStep = CoarseStep / 2.0;

	const double CoarseError = VelocityErrorNorm(StepOne(FRk4Integrator{}, Model, 0.0, Muzzle, CoarseStep).State,
												 Reference(Model, Muzzle, CoarseStep));
	const double FineError = VelocityErrorNorm(StepOne(FRk4Integrator{}, Model, 0.0, Muzzle, FineStep).State,
											   Reference(Model, Muzzle, FineStep));

	CHECK_THAT(std::log2(CoarseError / FineError), WithinAbs(5.0, 0.3));
}

TEST_CASE("Dormand-Prince error estimate falls as the fifth power of the step", "[integrator]")
{
	const FPointMassModel Model = MakeSmoothAirModel();
	constexpr double CoarseStep = 0.05;
	constexpr double FineStep = CoarseStep / 2.0;

	const double CoarseEstimate =
		Ogiva::Length(StepOne(FDormandPrinceIntegrator{}, Model, 0.0, Muzzle, CoarseStep).LocalError.Velocity);
	const double FineEstimate =
		Ogiva::Length(StepOne(FDormandPrinceIntegrator{}, Model, 0.0, Muzzle, FineStep).LocalError.Velocity);

	CHECK_THAT(std::log2(CoarseEstimate / FineEstimate), WithinAbs(5.0, 0.3));
}

TEST_CASE("Dormand-Prince is closer to the reference than RK4 at the same step", "[integrator]")
{
	const FPointMassModel Model = MakeSmoothAirModel();
	constexpr double TimeStep = 0.05;
	const FProjectileState Expected = Reference(Model, Muzzle, TimeStep);

	const double Rk4Error = VelocityErrorNorm(StepOne(FRk4Integrator{}, Model, 0.0, Muzzle, TimeStep).State, Expected);
	const double DormandPrinceError =
		VelocityErrorNorm(StepOne(FDormandPrinceIntegrator{}, Model, 0.0, Muzzle, TimeStep).State, Expected);

	CHECK(DormandPrinceError < Rk4Error / 10.0);
}

TEST_CASE("Only Dormand-Prince reports an error estimate order", "[integrator]")
{
	CHECK(FEulerIntegrator{}.ErrorEstimateOrder() == 0);
	CHECK(FRk4Integrator{}.ErrorEstimateOrder() == 0);
	CHECK(FDormandPrinceIntegrator{}.ErrorEstimateOrder() == 5);
}
