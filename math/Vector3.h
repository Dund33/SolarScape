#ifndef SOLARSCAPE_VECTOR3_H
#define SOLARSCAPE_VECTOR3_H

#include <cmath>
#include "math/Real.h"

struct Vector3
{
    Real x;
    Real y;
    Real z;

    Vector3();
    Vector3(Real xValue, Real yValue, Real zValue);

    Vector3 operator+(const Vector3& other) const
    {
        return {x + other.x, y + other.y, z + other.z};
    }

    Vector3 operator-(const Vector3& other) const
    {
        return {x - other.x, y - other.y, z - other.z};
    }

    Vector3 operator*(Real scalar) const
    {
        return {x * scalar, y * scalar, z * scalar};
    }

    Vector3 operator/(Real scalar) const
    {
        return {x / scalar, y / scalar, z / scalar};
    }

    Vector3& operator+=(const Vector3& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    Vector3& operator-=(const Vector3& other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    Vector3& operator*=(Real scalar)
    {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    Vector3& operator/=(Real scalar)
    {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        return *this;
    }

    Real lengthSquared() const
    {
        return x * x + y * y + z * z;
    }

    Real length() const
    {
        return std::sqrt(lengthSquared());
    }
};

inline Vector3 operator*(Real scalar, const Vector3& vector)
{
    return vector * scalar;
}

#endif
