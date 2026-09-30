#include "OgivaAim.h"
#include "OgivaDrag.h"
#include "OgivaFlight.h"

#include <catch_amalgamated.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using Ogiva::EError;
using Ogiva::FVector3;

namespace
{
	struct FReferenceRow
	{
		std::string Case;
		std::string DragModel;
		double BallisticCoefficient = 0.0;
		double MuzzleSpeed = 0.0;
		double SightHeight = 0.0;
		double ZeroRange = 0.0;
		double AirDensity = 0.0;
		double SpeedOfSound = 0.0;
		double ZeroElevation = 0.0;
		double Range = 0.0;
		double Time = 0.0;
		double Height = 0.0;
		double Windage = 0.0;
		double Speed = 0.0;
		double HeightConvergence = 0.0;
		double TimeConvergence = 0.0;
	};

	std::vector<FReferenceRow> ReadReference()
	{
		std::ifstream File(std::string{OGIVA_TEST_DATA_DIR} + "/trajectory/reference.csv");
		REQUIRE(File.is_open());
		std::string Line;
		std::getline(File, Line);
		std::vector<FReferenceRow> Rows;
		while (std::getline(File, Line))
		{
			std::istringstream Fields(Line);
			std::vector<std::string> Values;
			std::string Value;
			while (std::getline(Fields, Value, ','))
			{
				Values.push_back(Value);
			}
			REQUIRE(Values.size() == 16);
			auto Next = Values.cbegin();
			const auto Text = [&] { return *Next++; };
			const auto Number = [&] { return std::strtod((Next++)->c_str(), nullptr); };
			FReferenceRow Row;
			Row.Case = Text();
			Row.DragModel = Text();
			Row.BallisticCoefficient = Number();
			Row.MuzzleSpeed = Number();
			Row.SightHeight = Number();
			Row.ZeroRange = Number();
			Row.AirDensity = Number();
			Row.SpeedOfSound = Number();
			Row.ZeroElevation = Number();
			Row.Range = Number();
			Row.Time = Number();
			Row.Height = Number();
			Row.Windage = Number();
			Row.Speed = Number();
			Row.HeightConvergence = Number();
			Row.TimeConvergence = Number();
			Rows.push_back(Row);
		}
		return Rows;
	}

	Ogiva::FPointMassModel MakeModel(const FReferenceRow& Row)
	{
		const Ogiva::EDragModel Model = Row.DragModel == "G1" ? Ogiva::EDragModel::G1 : Ogiva::EDragModel::G7;
		return Ogiva::FPointMassModel({
			.DragCurve = &Ogiva::GetReferenceDragCurve(Model),
			.BallisticCoefficient = Ogiva::BallisticCoefficientFromImperial(Row.BallisticCoefficient),
			.AirDensity = Row.AirDensity,
			.SpeedOfSound = Row.SpeedOfSound,
			.Wind = FVector3::Zero(),
			.Gravity = {.X = 0.0, .Y = 0.0, .Z = -Ogiva::StandardGravity},
			.EarthRotation = FVector3::Zero(),
		});
	}

	constexpr Ogiva::FFlightSettings Settings{
		.StepControl = Ogiva::EStepControl::Adaptive,
		.TimeStep = 1e-3,
		.MaxTime = 10.0,
		.AbsoluteTolerance = 1e-12,
		.RelativeTolerance = 1e-12,
	};

	constexpr double HeightToleranceScale = 1e-7;
	constexpr double TimeTolerance = 2e-6;
	constexpr double SpeedTolerance = 1e-3;

	double HeightTolerance(double Range)
	{
		return HeightToleranceScale * Range * Range;
	}

	std::vector<FReferenceRow> ReadDownrangeRows()
	{
		std::vector<FReferenceRow> Rows = ReadReference();
		std::erase_if(Rows, [](const FReferenceRow& Row) { return Row.Range == 0.0; });
		return Rows;
	}

	Ogiva::FZeroRequest MakeRequest(const FReferenceRow& Row)
	{
		return {
			.Muzzle = FVector3::Zero(),
			.MuzzleSpeed = Row.MuzzleSpeed,
			.SightHeight = Row.SightHeight,
			.ZeroRange = Row.ZeroRange,
		};
	}

	void CheckReferenceFlight(const Ogiva::IIntegrator& Integrator, const FReferenceRow& Row)
	{
		CAPTURE(Row.Case, Row.Range);
		const Ogiva::FProjectileState Launch{
			.Position = FVector3::Zero(),
			.Velocity = Ogiva::LaunchDirection(Row.ZeroElevation, 0.0) * Row.MuzzleSpeed,
		};
		const Ogiva::FPlane Plane{.Normal = {.X = 1.0, .Y = 0.0, .Z = 0.0}, .Offset = Row.Range};
		Ogiva::FPlaneCrossing Crossing;
		REQUIRE(Ogiva::FlyToPlane(Integrator, MakeModel(Row), Launch, Plane, Settings, Crossing) == EError::None);

		CHECK(std::abs(Crossing.State.Position.Z - Row.SightHeight - Row.Height) <= HeightTolerance(Row.Range));
		CHECK(std::abs(Crossing.Time - Row.Time) <= TimeTolerance);
		CHECK(std::abs(Ogiva::Length(Crossing.State.Velocity) - Row.Speed) <= SpeedTolerance);
	}

	void CheckZeroAndHold(const Ogiva::IIntegrator& Integrator, const FReferenceRow& Row)
	{
		CAPTURE(Row.Case, Row.Range);
		const Ogiva::FPointMassModel Model = MakeModel(Row);
		const Ogiva::FZeroRequest Request = MakeRequest(Row);
		Ogiva::FAimSolution Zero;
		REQUIRE(Ogiva::SolveZero(Integrator, Model, Request, Settings, Zero) == EError::None);
		Ogiva::FHold Hold;
		REQUIRE(Ogiva::SolveHold(Integrator, Model, Request, Zero, Row.Range, Settings, Hold) == EError::None);

		CHECK(std::abs(Hold.Drop + Row.Height) <= HeightTolerance(Row.Range));
		CHECK(std::abs(Hold.Crossing.Time - Row.Time) <= TimeTolerance);
	}
}

TEST_CASE("The reference table covers both drag models out to 900 yards", "[reference]")
{
	const std::vector<FReferenceRow> Rows = ReadReference();
	for (const std::string Case : {"g7_308_175gr", "g1_308_150gr"})
	{
		const auto Count = std::ranges::count(Rows, Case, &FReferenceRow::Case);
		CHECK(Count == 10);
	}
}

TEST_CASE("Flying the reference zero elevation matches the reference drop table", "[reference]")
{
	const Ogiva::FDormandPrinceIntegrator Integrator;
	for (const FReferenceRow& Row : ReadDownrangeRows())
	{
		CheckReferenceFlight(Integrator, Row);
	}
}

TEST_CASE("Zeroing and holding reproduce the reference drop table", "[reference]")
{
	const Ogiva::FDormandPrinceIntegrator Integrator;
	for (const FReferenceRow& Row : ReadDownrangeRows())
	{
		CheckZeroAndHold(Integrator, Row);
	}
}
