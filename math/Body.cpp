//
// Created by Luke on 5/7/2026.
//

#include "Body.h"

Body::Body() :  mass(0.0L)
{
}

Body::Body(const Vector3& position, const Vector3& velocity, Real mass)
    : position(position), velocity(velocity), mass(mass)
{
}
