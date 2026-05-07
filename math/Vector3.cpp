//
// Created by Luke on 5/7/2026.
//

#include "Vector3.h"

#include <cmath>

Vector3::Vector3() : x(0.0L), y(0.0L), z(0.0L)
{
}

Vector3::Vector3(Real x, Real y, Real z) : x(x), y(y), z(z)
{
}

Vector3 Vector3::operator+(const Vector3& other) const
{
    return Vector3(x + other.x, y + other.y, z + other.z);
}

Vector3 Vector3::operator-(const Vector3& other) const
{
    return Vector3(x - other.x, y - other.y, z - other.z);
}

Vector3 Vector3::operator*(Real scalar) const
{
    return Vector3(x * scalar, y * scalar, z * scalar);
}

Vector3 Vector3::operator/(Real scalar) const
{
    return Vector3(x / scalar, y / scalar, z / scalar);
}

Vector3& Vector3::operator+=(const Vector3& other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
}

Vector3& Vector3::operator-=(const Vector3& other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
}

Vector3& Vector3::operator*=(Real scalar)
{
    x *= scalar;
    y *= scalar;
    z *= scalar;
    return *this;
}

Vector3& Vector3::operator/=(Real scalar)
{
    x /= scalar;
    y /= scalar;
    z /= scalar;
    return *this;
}

Real Vector3::lengthSquared() const
{
    return x * x + y * y + z * z;
}

Real Vector3::length() const
{
    return std::sqrt(lengthSquared());
}

Vector3 operator*(Real scalar, const Vector3& vector)
{
    return vector * scalar;
}
