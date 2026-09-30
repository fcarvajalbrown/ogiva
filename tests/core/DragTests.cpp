#include "OgivaDrag.h"

#include <catch_amalgamated.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

using Ogiva::EDragModel;

namespace
{
	struct FTable
	{
		std::vector<double> Mach;
		std::vector<double> DragCoefficient;
	};

	double ParseDouble(const std::string& Text)
	{
		return std::strtod(Text.c_str(), nullptr);
	}

	FTable ReadTable(const std::string& FileName)
	{
		std::ifstream File(std::string{OGIVA_TEST_DATA_DIR} + "/drag/" + FileName);
		REQUIRE(File.is_open());
		FTable Table;
		std::string MachText;
		std::string DragText;
		while (File >> MachText >> DragText)
		{
			Table.Mach.push_back(ParseDouble(MachText));
			Table.DragCoefficient.push_back(ParseDouble(DragText));
		}
		return Table;
	}

	void CheckMatchesSourceAtKnots(EDragModel Model, const FTable& Table)
	{
		auto Expected = Table.DragCoefficient.begin();
		for (const double Mach : Table.Mach)
		{
			CHECK(Ogiva::ReferenceDragCoefficient(Model, Mach) == *Expected);
			++Expected;
		}
	}

	void CheckStaysWithinEachInterval(EDragModel Model, const FTable& Table)
	{
		constexpr int SamplesPerInterval = 50;
		auto Coefficient = Table.DragCoefficient.begin();
		for (auto Mach = Table.Mach.begin(); std::next(Mach) != Table.Mach.end(); ++Mach, ++Coefficient)
		{
			const double Low = std::min(*Coefficient, *std::next(Coefficient));
			const double High = std::max(*Coefficient, *std::next(Coefficient));
			for (int Sample = 1; Sample < SamplesPerInterval; ++Sample)
			{
				const double X = *Mach + (*std::next(Mach) - *Mach) * Sample / SamplesPerInterval;
				const double Value = Ogiva::ReferenceDragCoefficient(Model, X);
				CHECK(Value >= Low);
				CHECK(Value <= High);
			}
		}
	}
}

TEST_CASE("G1 curve reproduces the source table at every knot", "[drag]")
{
	const FTable Table = ReadTable("mcg1.txt");
	REQUIRE(Table.Mach.size() == 79);
	CheckMatchesSourceAtKnots(EDragModel::G1, Table);
}

TEST_CASE("G7 curve reproduces the source table at every knot", "[drag]")
{
	const FTable Table = ReadTable("mcg7.txt");
	REQUIRE(Table.Mach.size() == 84);
	CheckMatchesSourceAtKnots(EDragModel::G7, Table);
}

TEST_CASE("Reference drag never overshoots its neighbouring knots", "[drag]")
{
	CheckStaysWithinEachInterval(EDragModel::G1, ReadTable("mcg1.txt"));
	CheckStaysWithinEachInterval(EDragModel::G7, ReadTable("mcg7.txt"));
}

TEST_CASE("Reference drag clamps outside Mach 0 to 5", "[drag]")
{
	CHECK(Ogiva::GetReferenceDragCurve(EDragModel::G1).MinX() == 0.0);
	CHECK(Ogiva::GetReferenceDragCurve(EDragModel::G1).MaxX() == 5.0);
	CHECK(Ogiva::ReferenceDragCoefficient(EDragModel::G1, 6.0) == 0.4988);
	CHECK(Ogiva::ReferenceDragCoefficient(EDragModel::G7, 6.0) == 0.1618);
	CHECK(Ogiva::ReferenceDragCoefficient(EDragModel::G7, -1.0) == 0.1198);
}

TEST_CASE("An unknown drag model evaluates to NaN", "[drag]")
{
	CHECK(std::isnan(Ogiva::ReferenceDragCoefficient(static_cast<EDragModel>(255), 1.0)));
}
