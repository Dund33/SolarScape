#include "Probe.h"

Probe::Probe() = default;

Probe::Probe(
    const Vector3& position,
    const Vector3& velocity,
    Real mass)
    : Body(position, velocity, mass),
      emptyMass_(mass)
{
}

Probe::Probe(
    const Vector3& position,
    const Vector3& velocity,
    Real emptyMass,
    Real fuelMass,
    Real fuelFlow,
    Real specificImpulse)
    : Body(position, velocity, emptyMass + fuelMass),
      emptyMass_(emptyMass),
      fuelMass_(fuelMass),
      fuelFlow_(fuelFlow),
      specificImpulse_(specificImpulse)
{
}

Probe::~Probe() = default;

auto Probe::mass() const -> Real
{
    return emptyMass_ + fuelMass_;
}

auto Probe::emptyMass() const -> Real
{
    return emptyMass_;
}

void Probe::setEmptyMass(Real emptyMass)
{
    emptyMass_ = emptyMass;
}

auto Probe::fuelMass() const -> Real
{
    return fuelMass_;
}

void Probe::setFuelMass(Real fuelMass)
{
    fuelMass_ = fuelMass;
}

auto Probe::fuelFlow() const -> Real
{
    return fuelFlow_;
}

void Probe::setFuelFlow(Real fuelFlow)
{
    fuelFlow_ = fuelFlow;
}

auto Probe::specificImpulse() const -> Real
{
    return specificImpulse_;
}

void Probe::setSpecificImpulse(Real specificImpulse)
{
    specificImpulse_ = specificImpulse;
}
