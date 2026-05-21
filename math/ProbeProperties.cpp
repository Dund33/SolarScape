#include "ProbeProperties.h"

#include <stdexcept>

ProbeProperties::ProbeProperties() = default;

ProbeProperties::ProbeProperties(
    Real emptyMass,
    Real fuelMass,
    Real fuelFlow,
    Real specificImpulse)
    : emptyMass_(emptyMass),
      fuelMass_(fuelMass),
      fuelFlow_(fuelFlow),
      specificImpulse_(specificImpulse)
{
    if (emptyMass < 0.0L)
    {
        throw std::invalid_argument("emptyMass must be non-negative.");
    }

    if (fuelMass < 0.0L)
    {
        throw std::invalid_argument("fuelMass must be non-negative.");
    }

    if (fuelFlow < 0.0L)
    {
        throw std::invalid_argument("fuelFlow must be non-negative.");
    }

    if (specificImpulse < 0.0L)
    {
        throw std::invalid_argument("specificImpulse must be non-negative.");
    }
}

auto ProbeProperties::emptyMass() const -> Real
{
    return emptyMass_;
}

auto ProbeProperties::fuelMass() const -> Real
{
    return fuelMass_;
}

auto ProbeProperties::fuelFlow() const -> Real
{
    return fuelFlow_;
}

auto ProbeProperties::specificImpulse() const -> Real
{
    return specificImpulse_;
}
