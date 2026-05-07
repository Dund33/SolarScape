//
// Created by Luke on 5/7/2026.
//

#include "Body.h"

Vector3::Vector3() : x(0.0), y(0.0), z(0.0)
{
}

Vector3::Vector3(double x, double y, double z) : x(x), y(y), z(z)
{
}

Body::Body() : position(), velocity(), mass(0.0)
{
}

Body::Body(const Vector3& position, const Vector3& velocity, double mass)
    : position(position), velocity(velocity), mass(mass)
{
}
