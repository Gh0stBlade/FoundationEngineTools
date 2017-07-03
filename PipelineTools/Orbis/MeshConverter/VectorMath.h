#pragma once

#include <math.h>

class Vector3
{
public:

	Vector3(float X = 0, float Y = 0, float Z = 0)
	{
		x = X;
		y = Y;
		z = Z;
	}

	~Vector3()
	{};

	float x, y, z;

	Vector3 operator*(float val) const
	{
		return Vector3(x * val, y * val, z * val);
	}

	friend Vector3 operator*(float val, Vector3 const &vec)
	{
		return Vector3(vec.x * val, vec.y * val, vec.z * val);
	}

	Vector3 operator+(const Vector3 &vec) const
	{
		return Vector3(x + vec.x, y + vec.y, z + vec.z);
	}

	Vector3 operator-(const Vector3 &vec) const
	{
		return Vector3(x - vec.x, y - vec.y, z - vec.z);
	}

	float GetMagnitude()
	{
		return std::sqrtf(x * x + y * y + z * z);
	}

	void normalize()
	{
		float magnitude = std::sqrtf(x * x + y * y + z * z);
		x /= magnitude;
		y /= magnitude;
		z /= magnitude;
	}

	float dot(const Vector3 &vec) const
	{
		return x * vec.x + y * vec.y + z * vec.z;
	}

	Vector3 cross(const Vector3 &vec) const
	{
		return Vector3(y * vec.z - z * vec.y, z * vec.x - x * vec.z, x * vec.y - y * vec.x);
	}
};