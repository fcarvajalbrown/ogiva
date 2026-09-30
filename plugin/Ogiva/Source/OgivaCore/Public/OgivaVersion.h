#pragma once

#include <cstdint>

namespace Ogiva
{
	struct FVersion
	{
		std::uint32_t Major = 0;
		std::uint32_t Minor = 0;
		std::uint32_t Patch = 0;

		friend bool operator==(const FVersion&, const FVersion&) = default;
	};

	[[nodiscard]] OGIVACORE_API FVersion GetVersion();
}
