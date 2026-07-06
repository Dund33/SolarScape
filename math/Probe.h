#ifndef SOLARSCAPE_PROBE_H
#define SOLARSCAPE_PROBE_H

#include "math/Body.h"

class Probe : public Body
{
public:
    using Body::Body;

    Probe();
    Probe(
        const Vector3& position,
        const Vector3& velocity,
        Real mass);
    Probe(
        const Vector3& position,
        const Vector3& velocity,
        Real emptyMass,
        Real fuelMass,
        Real fuelFlow,
        Real specificImpulse);
    ~Probe() override;

    auto mass() const -> Real override;

    auto emptyMass() const -> Real;
    void setEmptyMass(Real emptyMass);

    auto fuelMass() const -> Real;
    void setFuelMass(Real fuelMass);

    auto fuelFlow() const -> Real;
    void setFuelFlow(Real fuelFlow);

    auto specificImpulse() const -> Real;
    void setSpecificImpulse(Real specificImpulse);

private:
    Real emptyMass_{0.0};
    Real fuelMass_{0.0};
    Real fuelFlow_{0.0};
    Real specificImpulse_{0.0};
};


#endif
