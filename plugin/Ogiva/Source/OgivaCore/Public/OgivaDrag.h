#pragma once

#include "OgivaPchip.h"

#include <cstdint>

namespace Ogiva
{
	enum class EDragModel : std::uint8_t
	{
		G1,
		G7,
	};

	[[nodiscard]] OGIVACORE_API const FPchipCurve& GetReferenceDragCurve(EDragModel Model);
	[[nodiscard]] OGIVACORE_API double ReferenceDragCoefficient(EDragModel Model, double Mach);
}
