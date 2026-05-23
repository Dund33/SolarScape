#ifndef SOLARSCAPE_VERLETFACTORY_H
#define SOLARSCAPE_VERLETFACTORY_H

#include <vector>

#include "math/Body.h"
#include "math/ProbeFactory.h"
#include "simulation/SimulationFactory.h"

class VerletFactory final : public SimulationFactory
{
public:
    VerletFactory(
        std::vector<Body> bodies,
        Body targetBody,
        Vector3 probePosition,
        Vector3 probeVelocity,
        ProbeFactory probeFactory);

    std::unique_ptr<Simulation> create(
        SimulationContext context) const override;

private:
    std::vector<Body> bodies;
    Body targetBody;
    Vector3 probePosition;
    Vector3 probeVelocity;
    ProbeFactory probeFactory;
};

#endif
