#include "ProbeFactory.h"

#include <utility>

ProbeFactory::ProbeFactory(ProbeProperties properties)
    : properties_(std::move(properties))
{
}

auto ProbeFactory::create(
    const Vector3& position,
    const Vector3& velocity) const -> Probe
{
    return Probe(
        position,
        velocity,
        properties_.emptyMass(),
        properties_.fuelMass(),
        properties_.fuelFlow(),
        properties_.specificImpulse());
}

auto ProbeFactory::properties() const -> const ProbeProperties&
{
    return properties_;
}
