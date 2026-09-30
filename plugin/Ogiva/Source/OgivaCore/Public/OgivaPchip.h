#pragma once

#include "OgivaError.h"

#include <span>
#include <vector>

namespace Ogiva
{
	class FPchipCurve
	{
	public:
		[[nodiscard]] OGIVACORE_API static EError Create(std::span<const double> X, std::span<const double> Y,
														 FPchipCurve& OutCurve);

		[[nodiscard]] OGIVACORE_API double Evaluate(double X) const;
		[[nodiscard]] OGIVACORE_API double MinX() const;
		[[nodiscard]] OGIVACORE_API double MaxX() const;

	private:
		struct FSegment
		{
			double StartX = 0.0;
			double Cubic = 0.0;
			double Quadratic = 0.0;
			double Linear = 0.0;
			double Constant = 0.0;
		};

		std::vector<FSegment> Segments;
		double LastX = 0.0;
		double LastValue = 0.0;
	};
}
