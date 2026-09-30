#include "OgivaDrag.h"

#include "OgivaReferenceDragData.h"

namespace Ogiva
{
	namespace
	{
		FPchipCurve MakeCurve(std::span<const double> Mach, std::span<const double> DragCoefficient)
		{
			FPchipCurve Curve;
			static_cast<void>(FPchipCurve::Create(Mach, DragCoefficient, Curve));
			return Curve;
		}
	}

	const FPchipCurve& GetReferenceDragCurve(EDragModel Model)
	{
		static const FPchipCurve G1 = MakeCurve(ReferenceDragData::G1Mach, ReferenceDragData::G1DragCoefficient);
		static const FPchipCurve G7 = MakeCurve(ReferenceDragData::G7Mach, ReferenceDragData::G7DragCoefficient);
		static const FPchipCurve Empty;

		switch (Model)
		{
			case EDragModel::G1:
				return G1;
			case EDragModel::G7:
				return G7;
		}
		return Empty;
	}

	double ReferenceDragCoefficient(EDragModel Model, double Mach)
	{
		return GetReferenceDragCurve(Model).Evaluate(Mach);
	}
}
