#ifndef SOLARSCAPE_SIMULATIONFACTORY_H
#define SOLARSCAPE_SIMULATIONFACTORY_H

#include <memory>

#include "simulation/Simulation.h"

class SimulationFactory
{
public:
    virtual ~SimulationFactory() = default;

    virtual std::unique_ptr<Simulation> create() const = 0;
};

#endif
