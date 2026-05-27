#ifndef SOLARSCAPE_VERLETFACTORY_H
#define SOLARSCAPE_VERLETFACTORY_H

#include <vector>

#include "math/Body.h"
#include "math/Probe.h"
#include "simulation/SimulationFactory.h"

class VerletFactory final : public SimulationFactory
{
public:
    VerletFactory(
        Real gravitationalConstant,
        std::vector<Body> bodies,
        Body targetBody,
        Probe probe);

    std::unique_ptr<Simulation> create(
        SimulationContext context) const override;

private:
    Real gravitationalConstant;
    std::vector<Body> bodies;
    Body targetBody;
    Probe probe;
};

#endif
