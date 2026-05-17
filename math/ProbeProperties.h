#ifndef SOLARSCAPE_PROBEPROPERTIES_H
#define SOLARSCAPE_PROBEPROPERTIES_H

#include "math/Real.h"

class ProbeProperties
{
public:
    ProbeProperties();
    ProbeProperties(
        Real emptyMass,
        Real fuelMass,
        Real fuelFlow,
        Real specificImpulse);

    auto emptyMass() const -> Real;
    auto fuelMass() const -> Real;
    auto fuelFlow() const -> Real;
    auto specificImpulse() const -> Real;

private:
    Real emptyMass_{0.0L};
    Real fuelMass_{0.0L};
    Real fuelFlow_{0.0L};
    Real specificImpulse_{0.0L};
};

#endif
