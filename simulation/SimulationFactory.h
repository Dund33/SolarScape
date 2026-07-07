#ifndef SOLARSCAPE_SIMULATIONFACTORY_H
#define SOLARSCAPE_SIMULATIONFACTORY_H

#include <memory>
#include <vector>

#include "simulation/Maneuver.h"
#include "simulation/Simulation.h"

class SimulationFactory
{
public:
    virtual ~SimulationFactory() = default;

    virtual std::unique_ptr<Simulation> create(std::vector<Maneuver> maneuvers) const = 0;
};

#endif
