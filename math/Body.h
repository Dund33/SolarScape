#ifndef SOLARSCAPE_BODY_H
#define SOLARSCAPE_BODY_H

#include "math/Vector3.h"

class Body
{
public:
    Body();
    Body(const Vector3& position, const Vector3& velocity, Real mass);
    virtual ~Body() = default;

    auto position() -> Vector3&;
    auto position() const -> const Vector3&;

    auto velocity() -> Vector3&;
    auto velocity() const -> const Vector3&;

    virtual auto mass() const -> Real;

private:
    Vector3 position_;
    Vector3 velocity_;
    Real mass_;
};


#endif
