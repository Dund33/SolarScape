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

long double Vector3::norm() const
{
    return std::sqrt(
        static_cast<long double>(x) * static_cast<long double>(x) +
        static_cast<long double>(y) * static_cast<long double>(y) +
        static_cast<long double>(z) * static_cast<long double>(z)
    );
}

auto Vector3::operator+(const Vector3& other) const -> Vector3
{
    return {x + other.x, y + other.y, z + other.z};
}

auto Vector3::operator-(const Vector3& other) const -> Vector3
{
    return {x - other.x, y - other.y, z - other.z};
}

auto Vector3::operator*(Real scalar) const -> Vector3
{
    return {x * scalar, y * scalar, z * scalar};
}

auto Vector3::operator/(Real scalar) const -> Vector3
{
    return {x / scalar, y / scalar, z / scalar};
}

auto Vector3::operator+=(const Vector3& other) -> Vector3&
{
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
}

auto Vector3::operator-=(const Vector3& other) -> Vector3&
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
}

auto Vector3::operator*=(Real scalar) -> Vector3&
{
    x *= scalar;
    y *= scalar;
    z *= scalar;
    return *this;
}

auto Vector3::operator/=(Real scalar) -> Vector3&
{
    x /= scalar;
    y /= scalar;
    z /= scalar;
    return *this;
}

auto Vector3::lengthSquared() const -> Real
{
    return x * x + y * y + z * z;
}

auto Vector3::length() const -> Real
{
    return std::sqrt(lengthSquared());
}

auto operator*(Real scalar, const Vector3& vector) -> Vector3
{
    return vector * scalar;
}
