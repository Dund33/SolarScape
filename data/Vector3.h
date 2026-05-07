//
// Created by Luke on 5/7/2026.
//

#ifndef SOLARSCAPE_VECTOR3_H
#define SOLARSCAPE_VECTOR3_H

struct Vector3
{
    double x;
    double y;
    double z;

    Vector3();
    Vector3(double x, double y, double z);

    Vector3 operator+(const Vector3& other) const;
    Vector3 operator-(const Vector3& other) const;
    Vector3 operator*(double scalar) const;
    Vector3 operator/(double scalar) const;

    Vector3& operator+=(const Vector3& other);
    Vector3& operator-=(const Vector3& other);
    Vector3& operator*=(double scalar);
    Vector3& operator/=(double scalar);

    double lengthSquared() const;
    double length() const;
};

Vector3 operator*(double scalar, const Vector3& vector);

#endif //SOLARSCAPE_VECTOR3_H
