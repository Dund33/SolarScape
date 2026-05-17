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
        std::vector<Body> bodies,
        Body targetBody,
        Probe probe);

    std::unique_ptr<Simulation> create() const override;

private:
    std::vector<Body> bodies;
    Body targetBody;
    Probe probe;
};

#endif
