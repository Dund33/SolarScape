#ifndef SOLARSCAPE_VECTOR3_H
#define SOLARSCAPE_VECTOR3_H

#include "math/Real.h"

struct Vector3
{
    Real x;
    Real y;
    Real z;

    Vector3();
    Vector3(Real x, Real y, Real z);

    long double norm() const;
    
    Vector3 operator+(const Vector3& other) const;
    Vector3 operator-(const Vector3& other) const;
    Vector3 operator*(Real scalar) const;
    Vector3 operator/(Real scalar) const;

    Vector3& operator+=(const Vector3& other);
    Vector3& operator-=(const Vector3& other);
    Vector3& operator*=(Real scalar);
    Vector3& operator/=(Real scalar);

    Real lengthSquared() const;
    Real length() const;
};

Vector3 operator*(Real scalar, const Vector3& vector);

#endif
