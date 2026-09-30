#pragma once

#include <cmath>

namespace Ogiva
{
	struct FVector3
	{
		double X = 0.0;
		double Y = 0.0;
		double Z = 0.0;

		[[nodiscard]] static constexpr FVector3 Zero() { return {}; }
		[[nodiscard]] static constexpr FVector3 Up() { return {.X = 0.0, .Y = 0.0, .Z = 1.0}; }

		constexpr FVector3& operator+=(const FVector3& Other)
		{
			X += Other.X;
			Y += Other.Y;
			Z += Other.Z;
			return *this;
		}

		constexpr FVector3& operator-=(const FVector3& Other)
		{
			X -= Other.X;
			Y -= Other.Y;
			Z -= Other.Z;
			return *this;
		}

		constexpr FVector3& operator*=(double Scale)
		{
			X *= Scale;
			Y *= Scale;
			Z *= Scale;
			return *this;
		}

		constexpr FVector3& operator/=(double Divisor)
		{
			X /= Divisor;
			Y /= Divisor;
			Z /= Divisor;
			return *this;
		}

		[[nodiscard]] friend constexpr FVector3 operator+(FVector3 Left, const FVector3& Right)
		{
			return Left += Right;
		}
		[[nodiscard]] friend constexpr FVector3 operator-(FVector3 Left, const FVector3& Right)
		{
			return Left -= Right;
		}
		[[nodiscard]] friend constexpr FVector3 operator-(const FVector3& Vector)
		{
			return {.X = -Vector.X, .Y = -Vector.Y, .Z = -Vector.Z};
		}
		[[nodiscard]] friend constexpr FVector3 operator*(FVector3 Vector, double Scale) { return Vector *= Scale; }
		[[nodiscard]] friend constexpr FVector3 operator*(double Scale, FVector3 Vector) { return Vector *= Scale; }
		[[nodiscard]] friend constexpr FVector3 operator/(FVector3 Vector, double Divisor) { return Vector /= Divisor; }
		[[nodiscard]] friend constexpr bool operator==(const FVector3&, const FVector3&) = default;
	};

	[[nodiscard]] constexpr double Dot(const FVector3& Left, const FVector3& Right)
	{
		return Left.X * Right.X + Left.Y * Right.Y + Left.Z * Right.Z;
	}

	[[nodiscard]] constexpr FVector3 Cross(const FVector3& Left, const FVector3& Right)
	{
		return {
			.X = Left.Y * Right.Z - Left.Z * Right.Y,
			.Y = Left.Z * Right.X - Left.X * Right.Z,
			.Z = Left.X * Right.Y - Left.Y * Right.X,
		};
	}

	[[nodiscard]] constexpr double SquaredLength(const FVector3& Vector)
	{
		return Dot(Vector, Vector);
	}

	[[nodiscard]] inline double Length(const FVector3& Vector)
	{
		return std::sqrt(SquaredLength(Vector));
	}
}
