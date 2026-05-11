//
// Created by Luke on 5/7/2026.
//

#include "Body.h"

Body::Body() : mass_(0.0L)
{
}

Body::Body(const Vector3& position, const Vector3& velocity, Real mass)
    : position_(position), velocity_(velocity), mass_(mass)
{
}

auto Body::position() -> Vector3&
{
    return position_;
}

auto Body::position() const -> const Vector3&
{
    return position_;
}

auto Body::velocity() -> Vector3&
{
    return velocity_;
}

auto Body::velocity() const -> const Vector3&
{
    return velocity_;
}

auto Body::mass() const -> Real
{
    return mass_;
}
