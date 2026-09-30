#include "OgivaError.h"

namespace Ogiva
{
	std::string_view ToString(EError Error)
	{
		switch (Error)
		{
			case EError::None:
				return "None";
			case EError::InvalidArgument:
				return "InvalidArgument";
			case EError::OutOfRange:
				return "OutOfRange";
			case EError::NotConverged:
				return "NotConverged";
			case EError::TargetNotReached:
				return "TargetNotReached";
		}
		return "Unknown";
	}
}
