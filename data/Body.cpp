//
// Created by Luke on 5/7/2026.
//

#include "Body.h"

Body::Body() : position(), velocity(), mass(0.0)
{
}

Body::Body(const Vector3& position, const Vector3& velocity, double mass)
    : position(position), velocity(velocity), mass(mass)
{
}
