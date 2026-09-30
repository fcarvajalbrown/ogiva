#include "OgivaPchip.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iterator>
#include <limits>
#include <utility>

namespace Ogiva
{
	namespace
	{
		struct FInterval
		{
			double StartX = 0.0;
			double StartY = 0.0;
			double Step = 0.0;
			double Secant = 0.0;
		};

		bool IsValidInput(std::span<const double> X, std::span<const double> Y)
		{
			const auto IsFinite = [](double Value) { return std::isfinite(Value); };
			return X.size() >= 2 && X.size() == Y.size() && std::ranges::all_of(X, IsFinite) &&
				   std::ranges::all_of(Y, IsFinite) && std::ranges::adjacent_find(X, std::greater_equal{}) == X.end();
		}

		std::vector<FInterval> MakeIntervals(std::span<const double> X, std::span<const double> Y)
		{
			std::vector<FInterval> Intervals;
			Intervals.reserve(X.size() - 1);
			auto YIt = Y.begin();
			for (auto XIt = X.begin(); std::next(XIt) != X.end(); ++XIt, ++YIt)
			{
				const double Step = *std::next(XIt) - *XIt;
				Intervals.push_back({
					.StartX = *XIt,
					.StartY = *YIt,
					.Step = Step,
					.Secant = (*std::next(YIt) - *YIt) / Step,
				});
			}
			return Intervals;
		}

		double InteriorSlope(const FInterval& Left, const FInterval& Right)
		{
			if (Left.Secant * Right.Secant <= 0.0)
			{
				return 0.0;
			}
			const double LeftWeight = 2.0 * Right.Step + Left.Step;
			const double RightWeight = Right.Step + 2.0 * Left.Step;
			return (LeftWeight + RightWeight) / (LeftWeight / Left.Secant + RightWeight / Right.Secant);
		}

		double EndpointSlope(const FInterval& Near, const FInterval& Far)
		{
			const double Slope =
				((2.0 * Near.Step + Far.Step) * Near.Secant - Near.Step * Far.Secant) / (Near.Step + Far.Step);
			if (Slope * Near.Secant <= 0.0)
			{
				return 0.0;
			}
			if (Near.Secant * Far.Secant < 0.0 && std::fabs(Slope) > 3.0 * std::fabs(Near.Secant))
			{
				return 3.0 * Near.Secant;
			}
			return Slope;
		}

		std::vector<double> ComputeSlopes(const std::vector<FInterval>& Intervals)
		{
			std::vector<double> Slopes;
			Slopes.reserve(Intervals.size() + 1);
			if (Intervals.size() == 1)
			{
				Slopes.assign(2, Intervals.front().Secant);
				return Slopes;
			}
			Slopes.push_back(EndpointSlope(Intervals.front(), *std::next(Intervals.begin())));
			for (auto It = Intervals.begin(); std::next(It) != Intervals.end(); ++It)
			{
				Slopes.push_back(InteriorSlope(*It, *std::next(It)));
			}
			Slopes.push_back(EndpointSlope(Intervals.back(), *std::prev(Intervals.end(), 2)));
			return Slopes;
		}
	}

	EError FPchipCurve::Create(std::span<const double> X, std::span<const double> Y, FPchipCurve& OutCurve)
	{
		if (!IsValidInput(X, Y))
		{
			return EError::InvalidArgument;
		}

		const std::vector<FInterval> Intervals = MakeIntervals(X, Y);
		const std::vector<double> Slopes = ComputeSlopes(Intervals);

		FPchipCurve Curve;
		Curve.Segments.reserve(Intervals.size());
		auto StartSlope = Slopes.begin();
		for (const FInterval& Interval : Intervals)
		{
			const double EndSlope = *std::next(StartSlope);
			Curve.Segments.push_back({
				.StartX = Interval.StartX,
				.Cubic = (*StartSlope + EndSlope - 2.0 * Interval.Secant) / (Interval.Step * Interval.Step),
				.Quadratic = (3.0 * Interval.Secant - 2.0 * *StartSlope - EndSlope) / Interval.Step,
				.Linear = *StartSlope,
				.Constant = Interval.StartY,
			});
			++StartSlope;
		}
		Curve.LastX = X.back();
		Curve.LastValue = Y.back();

		OutCurve = std::move(Curve);
		return EError::None;
	}

	double FPchipCurve::Evaluate(double X) const
	{
		if (Segments.empty() || std::isnan(X))
		{
			return std::numeric_limits<double>::quiet_NaN();
		}
		if (X <= Segments.front().StartX)
		{
			return Segments.front().Constant;
		}
		if (X >= LastX)
		{
			return LastValue;
		}

		const FSegment& Segment = *std::prev(std::ranges::upper_bound(Segments, X, {}, &FSegment::StartX));
		const double Offset = X - Segment.StartX;
		return Segment.Constant + Offset * (Segment.Linear + Offset * (Segment.Quadratic + Offset * Segment.Cubic));
	}

	double FPchipCurve::MinX() const
	{
		return Segments.empty() ? std::numeric_limits<double>::quiet_NaN() : Segments.front().StartX;
	}

	double FPchipCurve::MaxX() const
	{
		return Segments.empty() ? std::numeric_limits<double>::quiet_NaN() : LastX;
	}
}
