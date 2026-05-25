#include "ProbeFactory.h"

#include <utility>

ProbeFactory::ProbeFactory(
    ProbeProperties properties,
    Vector3 position,
    Vector3 velocity)
    : properties_(std::move(properties)),
      position_(position),
      velocity_(velocity)
{
}

auto ProbeFactory::create() const -> Probe
{
    return Probe(
        position_,
        velocity_,
        properties_.emptyMass(),
        properties_.fuelMass(),
        properties_.fuelFlow(),
        properties_.specificImpulse());
}

auto ProbeFactory::properties() const -> const ProbeProperties&
{
    return properties_;
}
