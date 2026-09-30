#include "OgivaPchip.h"

#include <catch_amalgamated.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

using Catch::Matchers::WithinAbs;
using Ogiva::EError;
using Ogiva::FPchipCurve;

namespace
{
	FPchipCurve MakeCurve(const std::vector<double>& X, const std::vector<double>& Y)
	{
		FPchipCurve Curve;
		REQUIRE(FPchipCurve::Create(X, Y, Curve) == EError::None);
		return Curve;
	}

	double HermiteValue(double X, double StartX, double EndX, double StartY, double EndY, double StartSlope,
						double EndSlope)
	{
		const double Step = EndX - StartX;
		const double S = (X - StartX) / Step;
		const double S2 = S * S;
		const double S3 = S2 * S;
		return (2.0 * S3 - 3.0 * S2 + 1.0) * StartY + (S3 - 2.0 * S2 + S) * Step * StartSlope +
			   (-2.0 * S3 + 3.0 * S2) * EndY + (S3 - S2) * Step * EndSlope;
	}

	std::vector<double> SampleGrid(double From, double To, int Count)
	{
		std::vector<double> Grid;
		for (int Index = 0; Index <= Count; ++Index)
		{
			Grid.push_back(From + (To - From) * Index / Count);
		}
		return Grid;
	}
}

TEST_CASE("Create rejects malformed knots", "[pchip]")
{
	constexpr double Inf = std::numeric_limits<double>::infinity();
	const double NaN = std::nan("");
	FPchipCurve Curve;

	CHECK(FPchipCurve::Create(std::vector<double>{1.0}, std::vector<double>{1.0}, Curve) == EError::InvalidArgument);
	CHECK(FPchipCurve::Create(std::vector<double>{0.0, 1.0}, std::vector<double>{1.0}, Curve) ==
		  EError::InvalidArgument);
	CHECK(FPchipCurve::Create(std::vector<double>{0.0, 1.0, 1.0}, std::vector<double>{0.0, 1.0, 2.0}, Curve) ==
		  EError::InvalidArgument);
	CHECK(FPchipCurve::Create(std::vector<double>{0.0, 2.0, 1.0}, std::vector<double>{0.0, 1.0, 2.0}, Curve) ==
		  EError::InvalidArgument);
	CHECK(FPchipCurve::Create(std::vector<double>{0.0, NaN}, std::vector<double>{0.0, 1.0}, Curve) ==
		  EError::InvalidArgument);
	CHECK(FPchipCurve::Create(std::vector<double>{0.0, 1.0}, std::vector<double>{Inf, 1.0}, Curve) ==
		  EError::InvalidArgument);
}

TEST_CASE("A failed Create leaves the output curve unchanged", "[pchip]")
{
	FPchipCurve Curve = MakeCurve({0.0, 1.0}, {2.0, 4.0});

	CHECK(FPchipCurve::Create(std::vector<double>{1.0, 0.0}, std::vector<double>{0.0, 0.0}, Curve) ==
		  EError::InvalidArgument);
	CHECK(Curve.Evaluate(0.5) == 3.0);
}

TEST_CASE("A default curve evaluates to NaN", "[pchip]")
{
	const FPchipCurve Curve;
	CHECK(std::isnan(Curve.Evaluate(0.0)));
	CHECK(std::isnan(Curve.MinX()));
	CHECK(std::isnan(Curve.MaxX()));
}

TEST_CASE("Evaluate returns knot values exactly", "[pchip]")
{
	const std::vector<double> X{0.0, 0.3, 0.9, 1.0, 2.5};
	const std::vector<double> Y{0.2629, 0.2193, 0.2682, 0.4805, 0.2980};
	const FPchipCurve Curve = MakeCurve(X, Y);

	auto Value = Y.begin();
	for (const double Knot : X)
	{
		CHECK(Curve.Evaluate(Knot) == *Value);
		++Value;
	}
	CHECK(Curve.MinX() == 0.0);
	CHECK(Curve.MaxX() == 2.5);
}

TEST_CASE("Evaluate clamps outside the knots and propagates NaN", "[pchip]")
{
	const FPchipCurve Curve = MakeCurve({1.0, 2.0, 3.0}, {5.0, 7.0, 6.0});

	CHECK(Curve.Evaluate(-10.0) == 5.0);
	CHECK(Curve.Evaluate(0.999) == 5.0);
	CHECK(Curve.Evaluate(3.001) == 6.0);
	CHECK(Curve.Evaluate(1e300) == 6.0);
	CHECK(std::isnan(Curve.Evaluate(std::nan(""))));
}

TEST_CASE("Two knots give a straight line", "[pchip]")
{
	const FPchipCurve Curve = MakeCurve({1.0, 3.0}, {2.0, 6.0});
	for (const double X : SampleGrid(1.0, 3.0, 64))
	{
		CHECK_THAT(Curve.Evaluate(X), WithinAbs(2.0 * X, 1e-14));
	}
}

TEST_CASE("Linear data is reproduced on uneven knots", "[pchip]")
{
	const std::vector<double> X{0.0, 0.5, 2.0, 3.5, 4.0};
	std::vector<double> Y(X.size());
	std::ranges::transform(X, Y.begin(), [](double Knot) { return 2.0 * Knot - 1.0; });
	const FPchipCurve Curve = MakeCurve(X, Y);

	for (const double Sample : SampleGrid(0.0, 4.0, 400))
	{
		CHECK_THAT(Curve.Evaluate(Sample), WithinAbs(2.0 * Sample - 1.0, 1e-14));
	}
}

TEST_CASE("Slopes follow the weighted harmonic mean and shape-preserving endpoints", "[pchip]")
{
	const FPchipCurve Curve = MakeCurve({0.0, 1.0, 3.0}, {0.0, 1.0, 4.0});
	constexpr double StartSlope = 5.0 / 6.0;
	constexpr double MiddleSlope = 27.0 / 23.0;
	constexpr double EndSlope = 11.0 / 6.0;

	for (const double X : SampleGrid(0.0, 1.0, 16))
	{
		CHECK_THAT(Curve.Evaluate(X), WithinAbs(HermiteValue(X, 0.0, 1.0, 0.0, 1.0, StartSlope, MiddleSlope), 1e-15));
	}
	for (const double X : SampleGrid(1.0, 3.0, 16))
	{
		CHECK_THAT(Curve.Evaluate(X), WithinAbs(HermiteValue(X, 1.0, 3.0, 1.0, 4.0, MiddleSlope, EndSlope), 1e-14));
	}
}

TEST_CASE("Endpoint slope is limited to three times the secant when the data turns", "[pchip]")
{
	const FPchipCurve Curve = MakeCurve({0.0, 1.0, 1.1}, {0.0, 1.0, 0.0});
	constexpr double LimitedStartSlope = 3.0;
	constexpr double PeakSlope = 0.0;
	constexpr double EndSlope = -11.0;

	for (const double X : SampleGrid(0.0, 1.0, 16))
	{
		CHECK_THAT(Curve.Evaluate(X),
				   WithinAbs(HermiteValue(X, 0.0, 1.0, 0.0, 1.0, LimitedStartSlope, PeakSlope), 1e-15));
	}
	for (const double X : SampleGrid(1.0, 1.1, 16))
	{
		CHECK_THAT(Curve.Evaluate(X), WithinAbs(HermiteValue(X, 1.0, 1.1, 1.0, 0.0, PeakSlope, EndSlope), 1e-14));
	}
}

TEST_CASE("Monotone data stays monotone without overshoot across a sharp knee", "[pchip]")
{
	const FPchipCurve Curve = MakeCurve({0.0, 0.8, 0.9, 1.0, 1.2, 2.0}, {0.12, 0.13, 0.20, 0.40, 0.41, 0.42});

	double Previous = Curve.Evaluate(0.0);
	for (const double X : SampleGrid(0.0, 2.0, 4000))
	{
		const double Value = Curve.Evaluate(X);
		CHECK(Value >= Previous);
		CHECK(Value >= 0.12);
		CHECK(Value <= 0.42);
		Previous = Value;
	}
}

TEST_CASE("A local peak is not overshot", "[pchip]")
{
	const FPchipCurve Curve = MakeCurve({0.0, 1.0, 2.0, 3.0}, {0.0, 1.0, 0.5, 0.4});
	for (const double X : SampleGrid(0.0, 3.0, 3000))
	{
		CHECK(Curve.Evaluate(X) <= 1.0);
	}
	CHECK(Curve.Evaluate(1.0) == 1.0);
}

TEST_CASE("Constant data stays constant", "[pchip]")
{
	const FPchipCurve Curve = MakeCurve({0.0, 1.0, 4.0, 5.0}, {0.3, 0.3, 0.3, 0.3});
	for (const double X : SampleGrid(0.0, 5.0, 100))
	{
		CHECK(Curve.Evaluate(X) == 0.3);
	}
}
