#include "OgivaVersion.h"

namespace Ogiva
{
	FVersion GetVersion()
	{
		return {.Major = OGIVA_VERSION_MAJOR, .Minor = OGIVA_VERSION_MINOR, .Patch = OGIVA_VERSION_PATCH};
	}
}
