#pragma once

#include <cstdint>
#include <string_view>

namespace Ogiva
{
	enum class EError : std::uint8_t
	{
		None,
		InvalidArgument,
		OutOfRange,
		NotConverged,
		TargetNotReached,
	};

	[[nodiscard]] OGIVACORE_API std::string_view ToString(EError Error);
}
