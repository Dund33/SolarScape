#ifndef SOLARSCAPE_VERLETFACTORY_H
#define SOLARSCAPE_VERLETFACTORY_H

#include "simulation/SimulationFactory.h"

class VerletFactory final : public SimulationFactory
{
public:
    std::unique_ptr<Simulation> create() const override;
};

#endif
