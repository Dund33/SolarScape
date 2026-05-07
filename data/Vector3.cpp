//
// Created by Luke on 5/7/2026.
//

#include "Vector3.h"

#include <cmath>

Vector3::Vector3() : x(0.0), y(0.0), z(0.0)
{
}

Vector3::Vector3(double x, double y, double z) : x(x), y(y), z(z)
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

Vector3 Vector3::operator*(double scalar) const
{
    return Vector3(x * scalar, y * scalar, z * scalar);
}

Vector3 Vector3::operator/(double scalar) const
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

Vector3& Vector3::operator*=(double scalar)
{
    x *= scalar;
    y *= scalar;
    z *= scalar;
    return *this;
}

Vector3& Vector3::operator/=(double scalar)
{
    x /= scalar;
    y /= scalar;
    z /= scalar;
    return *this;
}

double Vector3::lengthSquared() const
{
    return x * x + y * y + z * z;
}

double Vector3::length() const
{
    return std::sqrt(lengthSquared());
}

Vector3 operator*(double scalar, const Vector3& vector)
{
    return vector * scalar;
}
